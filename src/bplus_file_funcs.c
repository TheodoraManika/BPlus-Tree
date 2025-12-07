#include "bplus_file_funcs.h"
#include "bplus_datanode.h"
#include "bplus_index_node.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bf.h"

#define bplus_OK 0
#define bplus_ERROR -1

#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return bplus_ERROR;     \
    }                         \
  }

/**
 * Helper function to get the maximum records that can fit in a data node
 */
static int get_max_records_per_node(const TableSchema *schema) {
  int data_area_size = BF_BLOCK_SIZE - 3 * sizeof(int);
  return data_area_size / schema->record_size;
}

/**
 * Helper function to get the maximum keys that can fit in an index node
 */
static int get_max_keys_per_index_node() {
  // Each entry is: key (int) + child pointer (int) = 8 bytes
  // Total space: num_keys * 8 + (num_keys + 1) * 4 for pointers
  int data_area_size = BF_BLOCK_SIZE - 2 * sizeof(int);
  return (data_area_size - sizeof(int)) / (2 * sizeof(int));
}

/**
 * Helper to get block counter value
 */
static int get_block_count(int file_desc) {
  int count = 0;
  BF_GetBlockCounter(file_desc, &count);
  return count;
}


int bplus_create_file(const TableSchema *schema, const char *fileName)
{
   CALL_BF(BF_CreateFile(fileName));

    int fd;
    CALL_BF(BF_OpenFile(fileName, &fd));

    // Block 0: metadata 
    BF_Block *meta_block;
    BF_Block_Init(&meta_block);

    CALL_BF(BF_AllocateBlock(fd, meta_block));
    char *data = BF_Block_GetData(meta_block);

    BPlusMeta meta;
    memcpy(&meta.schema, schema, sizeof(TableSchema));
    meta.root_block = 1;      // the root will be block 1 (data node initially)
    meta.height = 1;          // start with height 1 (only leaf level)
    meta.file_desc = fd;

    memcpy(data, &meta, sizeof(BPlusMeta));

    BF_Block_SetDirty(meta_block);
    CALL_BF(BF_UnpinBlock(meta_block));
    BF_Block_Destroy(&meta_block);

    // Block 1: root data node (leaf)
    BF_Block *root_block;
    BF_Block_Init(&root_block);

    CALL_BF(BF_AllocateBlock(fd, root_block));

    BPlusDataNode node;
    datanode_init(&node); // sets num_records=0, next_leaf=-1, prev_leaf=-1

    char *root_data = BF_Block_GetData(root_block);
    memcpy(root_data, &node, sizeof(BPlusDataNode));

    BF_Block_SetDirty(root_block);
    CALL_BF(BF_UnpinBlock(root_block));
    BF_Block_Destroy(&root_block);

    CALL_BF(BF_CloseFile(fd));
  return bplus_OK;
}


int bplus_open_file(const char *fileName, int *file_desc, BPlusMeta **metadata)
{
    CALL_BF(BF_OpenFile(fileName, file_desc));

    BF_Block *block;
    BF_Block_Init(&block);

    CALL_BF(BF_GetBlock(*file_desc, 0, block));
    char *data = BF_Block_GetData(block);

    *metadata = malloc(sizeof(BPlusMeta));
    memcpy(*metadata, data, sizeof(BPlusMeta));

//the file_desk must be updated because the one stored in metadata is garbage
    (*metadata)->file_desc = *file_desc;

    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);

  return bplus_OK;
}

int bplus_close_file(const int file_desc, BPlusMeta* metadata)
{
  free(metadata);
    CALL_BF(BF_CloseFile(file_desc));
  return bplus_OK;
}

/**
 * Helper: Insert a record into a leaf node (handles shifting for sorted insertion)
 * Returns: 0 if no split, 1 if split occurred
 * If split occurs, split_key and new_block_id are set
 */
static int insert_into_leaf(int file_desc, int leaf_block_id, const Record *record,
                           const TableSchema *schema, int *split_key, int *new_block_id) {
    BF_Block *leaf_block;
    BF_Block_Init(&leaf_block);
    CALL_BF(BF_GetBlock(file_desc, leaf_block_id, leaf_block));

    char *data = BF_Block_GetData(leaf_block);
    BPlusDataNode *leaf_node = (BPlusDataNode *)data;

    int num = datanode_get_num_records(leaf_node);
    int max_records = get_max_records_per_node(schema);
    int new_key = record_get_key(schema, record);

    // Find correct sorted position
    int pos = 0;
    for (int i = 0; i < num; i++) {
        Record tmp;
        datanode_get_record(leaf_node, i, schema, &tmp);
        int existing_key = record_get_key(schema, &tmp);
        if (existing_key > new_key) {
            pos = i;
            break;
        }
        pos = i + 1;
    }

    // Check if leaf is full
    if (num >= max_records) {
        // Need to split the leaf node
        BF_Block *new_leaf_block;
        BF_Block_Init(&new_leaf_block);
        CALL_BF(BF_AllocateBlock(file_desc, new_leaf_block));

        char *new_data = BF_Block_GetData(new_leaf_block);
        BPlusDataNode *new_leaf = (BPlusDataNode *)new_data;
        datanode_init(new_leaf);

        int mid = max_records / 2;

        // Copy second half of records to new leaf
        for (int i = mid; i < num; i++) {
            Record tmp;
            datanode_get_record(leaf_node, i, schema, &tmp);
            datanode_set_record(new_leaf, i - mid, schema, &tmp);
        }
        new_leaf->num_records = num - mid;

        // Update leaf node to have only first half
        leaf_node->num_records = mid;

        // Update leaf pointers
        new_leaf->next_leaf = leaf_node->next_leaf;
        new_leaf->prev_leaf = leaf_block_id;
        leaf_node->next_leaf = get_block_count(file_desc) - 1;

        // Insert the new record into the appropriate leaf
        if (pos < mid) {
            // Insert into left leaf
            for (int i = leaf_node->num_records; i > pos; i--) {
                Record tmp;
                datanode_get_record(leaf_node, i - 1, schema, &tmp);
                datanode_set_record(leaf_node, i, schema, &tmp);
            }
            datanode_set_record(leaf_node, pos, schema, record);
            leaf_node->num_records++;
        } else {
            // Insert into right leaf
            int new_pos = pos - mid;
            for (int i = new_leaf->num_records; i > new_pos; i--) {
                Record tmp;
                datanode_get_record(new_leaf, i - 1, schema, &tmp);
                datanode_set_record(new_leaf, i, schema, &tmp);
            }
            datanode_set_record(new_leaf, new_pos, schema, record);
            new_leaf->num_records++;
        }

        // Get the first key of the right leaf for parent
    Record first_right;
    datanode_get_record(new_leaf, 0, schema, &first_right);
        *split_key = record_get_key(schema, &first_right);
        *new_block_id = get_block_count(file_desc) - 1;

        BF_Block_SetDirty(leaf_block);
        CALL_BF(BF_UnpinBlock(leaf_block));
        BF_Block_Destroy(&leaf_block);

        BF_Block_SetDirty(new_leaf_block);
        CALL_BF(BF_UnpinBlock(new_leaf_block));
        BF_Block_Destroy(&new_leaf_block);

        return 1; // Split occurred
    } else {
        // Leaf not full, just insert
        for (int i = num; i > pos; i--) {
            Record tmp;
            datanode_get_record(leaf_node, i - 1, schema, &tmp);
            datanode_set_record(leaf_node, i, schema, &tmp);
        }
        datanode_set_record(leaf_node, pos, schema, record);

        BF_Block_SetDirty(leaf_block);
        CALL_BF(BF_UnpinBlock(leaf_block));
        BF_Block_Destroy(&leaf_block);

        return 0; // No split
    }
}

/**
 * Helper: Insert a key-pointer pair into an index node
 * Returns: 0 if no split, 1 if split occurred
 * If split occurs, split_key and new_block_id are set
 */
static int insert_into_index(int file_desc, int index_block_id, int key, int left_child, int right_child,
                            int *split_key, int *new_block_id) {
    BF_Block *index_block;
    BF_Block_Init(&index_block);
    CALL_BF(BF_GetBlock(file_desc, index_block_id, index_block));

    char *data = BF_Block_GetData(index_block);
    BPlusIndexNode *index_node = (BPlusIndexNode *)data;

    int num_keys = indexnode_get_num_keys(index_node);
    int max_keys = get_max_keys_per_index_node();

    // Check if index is full
    if (num_keys >= max_keys) {
        // Need to split the index node
        BF_Block *new_index_block;
        BF_Block_Init(&new_index_block);
        CALL_BF(BF_AllocateBlock(file_desc, new_index_block));

        char *new_data = BF_Block_GetData(new_index_block);
        BPlusIndexNode *new_index = (BPlusIndexNode *)new_data;
        indexnode_init(new_index, 0); // Internal node

        int mid = (num_keys + 1) / 2;

        // Copy second half of keys and pointers to new index
        for (int i = mid + 1; i < num_keys; i++) {
            int k = indexnode_get_key(index_node, i);
            indexnode_set_key(new_index, i - mid - 1, k);
        }

        // Copy pointers
        for (int i = mid + 1; i <= num_keys; i++) {
            int p = indexnode_get_pointer(index_node, i);
            indexnode_set_pointer(new_index, i - mid - 1, p);
        }

        new_index->num_keys = num_keys - mid - 1;
        *split_key = indexnode_get_key(index_node, mid);
        index_node->num_keys = mid;
        *new_block_id = get_block_count(file_desc) - 1;

        // Now insert the new key into appropriate index node
        if (key < *split_key) {
            indexnode_add_key_pointer(index_node, key, left_child, right_child);
        } else {
            indexnode_add_key_pointer(new_index, key, left_child, right_child);
        }

        BF_Block_SetDirty(index_block);
        CALL_BF(BF_UnpinBlock(index_block));
        BF_Block_Destroy(&index_block);

        BF_Block_SetDirty(new_index_block);
        CALL_BF(BF_UnpinBlock(new_index_block));
        BF_Block_Destroy(&new_index_block);

        return 1; // Split occurred
    } else {
        // Index not full, just insert
        indexnode_add_key_pointer(index_node, key, left_child, right_child);

        BF_Block_SetDirty(index_block);
        CALL_BF(BF_UnpinBlock(index_block));
        BF_Block_Destroy(&index_block);

        return 0; // No split
    }
}

/**
 * Recursive helper: Insert record into tree, handling splits
 * current_level: current depth in tree (starts at metadata->height, decrements going down)
 */
static int insert_recursive(int file_desc, BPlusMeta *metadata, int current_block_id,
                           const Record *record, int *split_key, int *new_block_id, int current_level) {
    BF_Block *block;
    BF_Block_Init(&block);
    CALL_BF(BF_GetBlock(file_desc, current_block_id, block));
    char *data = BF_Block_GetData(block);

    // Check if this is a leaf node (level == 1)
    if (current_level == 1) {
        CALL_BF(BF_UnpinBlock(block));
        BF_Block_Destroy(&block);
        return insert_into_leaf(file_desc, current_block_id, record, &metadata->schema, split_key, new_block_id);
    }

    // This is an internal index node
    BPlusIndexNode *index_node = (BPlusIndexNode *)data;
    int key = record_get_key(&metadata->schema, record);
    int child_block = indexnode_find_child(index_node, key);

    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);

    // Recursively insert into child (decrement level)
    int child_split_key = -1;
    int child_new_block = -1;
    int child_split = insert_recursive(file_desc, metadata, child_block, record, &child_split_key, &child_new_block, current_level - 1);

    if (child_split == 0) {
        // No split in child, we're done
        return 0;
    }

    // Child split occurred, insert new key-pointer pair into current index node
    return insert_into_index(file_desc, current_block_id, child_split_key, child_block, child_new_block, split_key, new_block_id);
}

/**
 * @brief Inserts a record into the B+ tree.
 */
int bplus_record_insert(const int file_desc, BPlusMeta *metadata, const Record *record)
{
    if (!metadata || !record) return bplus_ERROR;

    int split_key = -1;
    int new_block_id = -1;
    int split_occurred = insert_recursive(file_desc, metadata, metadata->root_block, record, &split_key, &new_block_id, metadata->height);

    if (split_occurred == 0) {
        // No split at root, we're done
        return bplus_OK;
    }

    // Root split occurred, create new root index node
    BF_Block *new_root_block;
    BF_Block_Init(&new_root_block);
    CALL_BF(BF_AllocateBlock(file_desc, new_root_block));

    char *new_root_data = BF_Block_GetData(new_root_block);
    BPlusIndexNode *new_root = (BPlusIndexNode *)new_root_data;
    indexnode_init(new_root, 0); // Internal node (not a leaf)

    // Add the split key and pointers
    indexnode_set_pointer(new_root, 0, metadata->root_block);
    indexnode_set_key(new_root, 0, split_key);
    indexnode_set_pointer(new_root, 1, new_block_id);
    new_root->num_keys = 1;

    BF_Block_SetDirty(new_root_block);
    CALL_BF(BF_UnpinBlock(new_root_block));
    BF_Block_Destroy(&new_root_block);

    // Update metadata
    metadata->root_block = get_block_count(file_desc) - 1;
    metadata->height++;

    // Update metadata block
    BF_Block *meta_block;
    BF_Block_Init(&meta_block);
    CALL_BF(BF_GetBlock(file_desc, 0, meta_block));
    char *meta_data = BF_Block_GetData(meta_block);
    memcpy(meta_data, metadata, sizeof(BPlusMeta));
    BF_Block_SetDirty(meta_block);
    CALL_BF(BF_UnpinBlock(meta_block));
    BF_Block_Destroy(&meta_block);

    return bplus_OK;
}

/**
 * @brief Finds a record in the B+ tree by key.
 */
int bplus_record_find(const int file_desc, const BPlusMeta *metadata, const int key, Record** out_record)
{
    int current_block = metadata->root_block;
    int height = metadata->height;

    // Traverse down the tree
    while (height > 1) {
        BF_Block *block;
        BF_Block_Init(&block);
        CALL_BF(BF_GetBlock(file_desc, current_block, block));
        char *data = BF_Block_GetData(block);

        BPlusIndexNode *index_node = (BPlusIndexNode *)data;
        current_block = indexnode_find_child(index_node, key);

        CALL_BF(BF_UnpinBlock(block));
        BF_Block_Destroy(&block);

        height--;
    }

    // Now we're at a leaf node
    BF_Block *leaf_block;
    BF_Block_Init(&leaf_block);
    CALL_BF(BF_GetBlock(file_desc, current_block, leaf_block));
    char *leaf_data = BF_Block_GetData(leaf_block);

    BPlusDataNode *leaf_node = (BPlusDataNode *)leaf_data;
    int num = datanode_get_num_records(leaf_node);

    Record *tmp = malloc(sizeof(Record));
    if (tmp == NULL) {
        CALL_BF(BF_UnpinBlock(leaf_block));
        BF_Block_Destroy(&leaf_block);
        return bplus_ERROR;
    }

    for (int i = 0; i < num; i++) {
        datanode_get_record(leaf_node, i, &metadata->schema, tmp);

        if (record_get_key(&metadata->schema, tmp) == key) {
            *out_record = tmp;
            CALL_BF(BF_UnpinBlock(leaf_block));
            BF_Block_Destroy(&leaf_block);
            return bplus_OK;
        }
    }

    // Record not found
    free(tmp);
    CALL_BF(BF_UnpinBlock(leaf_block));
    BF_Block_Destroy(&leaf_block);

    *out_record = NULL;
    return bplus_ERROR;
}