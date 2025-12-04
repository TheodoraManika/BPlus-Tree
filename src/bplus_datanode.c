#include "bplus_datanode.h"
#include "record.h"
#include <string.h>


/**
 * @brief Initializes an empty data node.
 * @param node Pointer to the data node to initialize.
 */
void datanode_init(BPlusDataNode *node) {
    node->num_records = 0;
    node->next_leaf = -1;
    node->prev_leaf = -1;
    memset(node->data, 0, sizeof(node->data));
}

/**
 * @brief Gets the number of records in a data node.
 * @param node Pointer to the data node.
 * @return Number of records in the node.
 */
int datanode_get_num_records(const BPlusDataNode *node) {
    return node->num_records;
}

/**
 * Returns the maximum number of records that fit in this node.
 */
int datanode_max_records(const BPlusDataNode *node) {
   return sizeof(node->data) / sizeof(Record);
}

/**
 * @brief Gets a record from a data node at a specific position.
 * @param node Pointer to the data node.
 * @param pos Position of the record (0-based).
 * @param schema Pointer to the table schema.
 * @param out_record Pointer to store the retrieved record.
 * @return 0 on success, -1 on failure.
 */
int datanode_get_record(const BPlusDataNode *node, int pos, Record *out_record) {
    if (pos < 0 || pos >= node->num_records) return -1;
    memcpy(out_record, node->data + pos * sizeof(Record), sizeof(Record));
    return 0;
}

/**
 * Split a full leaf node
 * @param old_node The leaf node to split
 * @param new_node Pointer to newly allocated node
 * @return index of first record in new_node (the split key for parent)
 */
int datanode_split(BPlusDataNode *old_node, BPlusDataNode *new_node) {
    datanode_init(new_node);

    int total = old_node->num_records;
    int mid = total / 2;  // split in half

    // Copy upper half to new node
    int move_count = total - mid;
    memcpy(new_node->data, old_node->data + mid * sizeof(Record), move_count * sizeof(Record));
    new_node->num_records = move_count;
    old_node->num_records = mid;

    // Update leaf links
    new_node->next_leaf = old_node->next_leaf;
    new_node->prev_leaf = -1; // will be updated by caller
    old_node->next_leaf = -1; // will be updated by caller

    // Return key for parent (first record in new node)
    Record *split_key = (Record *)new_node->data;
    return split_key->values[0].int_value; // assumes primary key is int
}


/**
 * @brief Sets a record in a data node at a specific position.
 * @param node Pointer to the data node.
 * @param pos Position to insert at (0-based).
 * @param record Pointer to the record to set.
 * @param schema Pointer to the table schema.
 * @return 0 on success, -1 on failure.
 */
int datanode_set_record(BPlusDataNode *node, int pos, const Record *record) {
    if (pos < 0 || pos >= datanode_max_records(node) ) 
    return -1;
    memcpy(node->data + pos * sizeof(Record), record, sizeof(Record));
    if (pos >= node->num_records) node->num_records = pos + 1;
    return 0;
}

/**
 * @brief Adds a record to a data node (assumes space is available).
 * @param node Pointer to the data node.
 * @param record Pointer to the record to add.
 * @param schema Pointer to the table schema.
 * @return 0 on success, -1 on failure.
 */
int datanode_add_record(BPlusDataNode *node, const Record *record) {
    if (node->num_records >= datanode_max_records(node))
        return -1; // node full
    return datanode_set_record(node, node->num_records, record);
}

/**
 * @brief Gets the next leaf block ID.
 * @param node Pointer to the data node.
 * @return Block ID of the next leaf, or -1 if none.
 */
int datanode_get_next_leaf(const BPlusDataNode *node) {
    return node->next_leaf;
}

/**
 * @brief Sets the next leaf block ID.
 * @param node Pointer to the data node.
 * @param next_leaf Block ID of the next leaf.
 */
void datanode_set_next_leaf(BPlusDataNode *node, int next_leaf) {
    node->next_leaf = next_leaf;
}

/**
 * @brief Gets the previous leaf block ID.
 * @param node Pointer to the data node.
 * @return Block ID of the previous leaf, or -1 if none.
 */
int datanode_get_prev_leaf(const BPlusDataNode *node) {
    return node->prev_leaf;
}


/**
 * @brief Sets the previous leaf block ID.
 * @param node Pointer to the data node.
 * @param prev_leaf Block ID of the previous leaf.
 */
void datanode_set_prev_leaf(BPlusDataNode *node, int prev_leaf) {
    node->prev_leaf = prev_leaf;
}