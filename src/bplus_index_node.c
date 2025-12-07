#include "bplus_index_node.h"
#include "bf.h"
#include <string.h>
#include <stdlib.h>

/**
 * Helper function to compute max_keys based on the data area size
 */
static int compute_max_keys() {
    int data_size = BF_BLOCK_SIZE - 2 * sizeof(int);  // same as sizeof(node->data)
    return (data_size - sizeof(int)) / (2 * sizeof(int));
}

/**
 * @brief Initializes an empty index node.
 * @param node Pointer to the index node to initialize.
 * @param is_leaf 1 if this is a leaf node, 0 if internal index node.
 */
void indexnode_init(BPlusIndexNode *node, int is_leaf) {
    node->num_keys = 0;
    node->is_leaf = is_leaf;
    // max_keys is computed on-the-fly, not stored
    node->max_keys = compute_max_keys();
    memset(node->data, 0, sizeof(node->data));
}

/**
 * @brief Gets the number of keys in an index node.
 * @param node Pointer to the index node.
 * @return Number of keys in the node.
 */
int indexnode_get_num_keys(const BPlusIndexNode *node) {
    return node->num_keys;
}

/**
 * @brief Gets the maximum number of keys the node can hold.
 * @param node Pointer to the index node.
 * @return Maximum number of keys.
 */
int indexnode_get_max_keys(const BPlusIndexNode *node) {
    (void)node;  // unused, max_keys is always computed
    return compute_max_keys();
}

/**
 * @brief Gets a key from an index node at a specific position.
 * @param node Pointer to the index node.
 * @param pos Position of the key (0-based).
 * @return The key value, or -1 on failure.
 */
int indexnode_get_key(const BPlusIndexNode *node, int pos) {
    if (pos < 0 || pos >= node->num_keys) {
        return -1; // Invalid position
    }

    int key;
    memcpy(&key, node->data + pos * sizeof(int), sizeof(int));
    return key;
}

/**
 * @brief Gets a pointer (block ID) from an index node at a specific position.
 * @param node Pointer to the index node.
 * @param pos Position of the pointer (0-based).
 * @return The block ID, or -1 on failure.
 */
int indexnode_get_pointer(const BPlusIndexNode *node, int pos) {
    int max_keys = compute_max_keys();
    if (pos < 0 || pos > max_keys) {
        return -1; // Invalid position
    }

    int pointer;
    /* pointers are stored after the keys area which is max_keys * sizeof(int) */
    memcpy(&pointer, node->data + (max_keys * sizeof(int)) + pos * sizeof(int), sizeof(int));
    return pointer;
}

/**
 * @brief Sets a key in an index node at a specific position.
 * @param node Pointer to the index node.
 * @param pos Position of the key (0-based).
 * @param key The key value to set.
 * @return 0 on success, -1 on failure.
 */
int indexnode_set_key(BPlusIndexNode *node, int pos, int key) {
    int max_keys = compute_max_keys();
    if (pos < 0 || pos >= max_keys) {
        return -1; // Invalid position
    }

    memcpy(node->data + pos * sizeof(int), &key, sizeof(int));
    return 0;
}

/**
 * @brief Sets a pointer (block ID) in an index node at a specific position.
 * @param node Pointer to the index node.
 * @param pos Position of the pointer (0-based).
 * @param block_id The block ID to set.
 * @return 0 on success, -1 on failure.
 */
int indexnode_set_pointer(BPlusIndexNode *node, int pos, int block_id) {
    int max_keys = compute_max_keys();
    if (pos < 0 || pos > max_keys) {
        return -1; // Invalid position
    }

    memcpy(node->data + (max_keys * sizeof(int)) + pos * sizeof(int), &block_id, sizeof(int));
    return 0;
}

/**
 * @brief Adds a key-pointer pair to an index node (assumes space is available).
 * @param node Pointer to the index node.
 * @param key The key value.
 * @param left_child The left child block ID.
 * @param right_child The right child block ID (for insertion after).
 * @return 0 on success, -1 on failure.
 */
int indexnode_add_key_pointer(BPlusIndexNode *node, int key, int left_child, int right_child) {
    int max_keys = compute_max_keys();
    if (node->num_keys >= max_keys) {
        return -1; // No space available
    }

    /* Read existing keys and pointers into temporaries */
    int keys[max_keys];
    int ptrs[max_keys + 1];
    for (int i = 0; i < node->num_keys; i++) keys[i] = indexnode_get_key(node, i);
    for (int i = 0; i <= node->num_keys; i++) ptrs[i] = indexnode_get_pointer(node, i);

    // Find insertion position
    int pos = 0;
    while (pos < node->num_keys && keys[pos] < key) pos++;

    // Shift in-memory arrays
    for (int i = node->num_keys; i > pos; i--) keys[i] = keys[i - 1];
    for (int i = node->num_keys + 1; i > pos + 1; i--) ptrs[i] = ptrs[i - 1];

    // Insert new key and pointers
    keys[pos] = key;
    ptrs[pos] = left_child;
    ptrs[pos + 1] = right_child;

    // Write back into node data area
    for (int i = 0; i <= max_keys - 1; i++) {
        int k = (i < node->num_keys + 1) ? keys[i] : 0;
        memcpy(node->data + i * sizeof(int), &k, sizeof(int));
    }
    for (int i = 0; i <= max_keys; i++) {
        int p = (i < node->num_keys + 2) ? ptrs[i] : 0;
        memcpy(node->data + (max_keys * sizeof(int)) + i * sizeof(int), &p, sizeof(int));
    }

    node->num_keys++;
    return 0;
}

/**
 * @brief Finds the child block ID to descend to for a given key.
 * @param node Pointer to the index node.
 * @param key The key to search for.
 * @return The child block ID to descend to, or -1 on failure.
 */
int indexnode_find_child(const BPlusIndexNode *node, int key) {
    int pos = 0;
    while (pos < node->num_keys && indexnode_get_key(node, pos) <= key) pos++;
    return indexnode_get_pointer(node, pos);
}

/**
 * @brief Gets whether this is a leaf node.
 * @param node Pointer to the index node.
 * @return 1 if leaf node, 0 if internal index node.
 */
int indexnode_is_leaf(const BPlusIndexNode *node) {
    return node->is_leaf;
}

int indexnode_split(BPlusIndexNode *old_node, BPlusIndexNode *new_node) {
    int total_keys = old_node->num_keys;
    int mid_index = total_keys / 2;

    int middle_key = indexnode_get_key(old_node, mid_index);

   
    int j = 0;
    for (int i = mid_index + 1; i < total_keys; i++, j++) {
        int key = indexnode_get_key(old_node, i);
        int ptr = indexnode_get_pointer(old_node, i + 1);
        indexnode_set_key(new_node, j, key);
        indexnode_set_pointer(new_node, j, ptr);
    }

    new_node->num_keys = total_keys - mid_index - 1;
    old_node->num_keys = mid_index;

    return middle_key;
}
