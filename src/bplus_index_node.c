#include "bplus_index_node.h"
#include "bf.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief Initializes an empty index node.
 * @param node Pointer to the index node to initialize.
 * @param is_leaf 1 if this is a leaf node, 0 if internal index node.
 */
void indexnode_init(BPlusIndexNode *node, int is_leaf) {
    node->num_keys = 0;
    node->is_leaf = is_leaf;
    // Calculate max_keys based on available data space
    // Each entry needs 1 key (int) and we need num_keys + 1 pointers (ints)
    // So total: num_keys * sizeof(int) + (num_keys + 1) * sizeof(int) <= data size
    // Solving: num_keys * (2 * sizeof(int)) + sizeof(int) <= data size
    node->max_keys = (sizeof(node->data) - sizeof(int)) / (2 * sizeof(int));
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
    return node->max_keys;
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
    if (pos < 0 || pos > node->num_keys) {
        return -1; // Invalid position
    }

    int pointer;
    memcpy(&pointer, node->data + (node->num_keys * sizeof(int)) + pos * sizeof(int), sizeof(int));
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
    if (pos < 0 || pos >= node->num_keys) {
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
    if (pos < 0 || pos > node->num_keys) {
        return -1; // Invalid position
    }

    memcpy(node->data + (node->num_keys * sizeof(int)) + pos * sizeof(int), &block_id, sizeof(int));
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
    if (node->num_keys >= node->max_keys) {
        return -1; // No space available
    }

    // Find position to insert the new key
    int pos = 0;
    while (pos < node->num_keys && indexnode_get_key(node, pos) < key) {
        pos++;
    }

    // Shift keys and pointers to make space
    for (int i = node->num_keys; i > pos; i--) {
        indexnode_set_key(node, i, indexnode_get_key(node, i - 1));
        indexnode_set_pointer(node, i + 1, indexnode_get_pointer(node, i));
    }

    // Insert new key and pointers
    indexnode_set_key(node, pos, key);
    indexnode_set_pointer(node, pos, left_child);
    indexnode_set_pointer(node, pos + 1, right_child);

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
    while (pos < node->num_keys && indexnode_get_key(node, pos) <= key) {
        pos++;
    }
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