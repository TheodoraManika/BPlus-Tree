#ifndef BP_INDEX_NODE_H
#define BP_INDEX_NODE_H

#include "bf.h"

/**
 * @brief Represents an index node in the B+ tree.
 * Index nodes contain keys and block pointers.
 */
typedef struct {
  int num_keys;                                 /**< Number of keys in this index node */
  int is_leaf;                                  /**< 1 if this is a leaf node, 0 if internal index node */
  char data[BF_BLOCK_SIZE - 2 * sizeof(int)];  /**< Keys and pointers data */
  int max_keys;                              /**< Maximum number of keys the node can hold */
} BPlusIndexNode;

/**
 * @brief Initializes an empty index node.
 */
void indexnode_init(BPlusIndexNode *node, int is_leaf);

/**
 * @brief Gets the number of keys in an index node.
 */
int indexnode_get_num_keys(const BPlusIndexNode *node);

/**
 * @brief Gets a key from an index node at a specific position.
 */
int indexnode_get_key(const BPlusIndexNode *node, int pos);

/**
 * @brief Gets a pointer (block ID) from an index node at a specific position.
 */
int indexnode_get_pointer(const BPlusIndexNode *node, int pos);

/**
 * @brief Sets a key in an index node at a specific position.
 */
int indexnode_set_key(BPlusIndexNode *node, int pos, int key);

/**
 * @brief Sets a pointer (block ID) in an index node at a specific position.
 */
int indexnode_set_pointer(BPlusIndexNode *node, int pos, int block_id);

/**
 * @brief Adds a key-pointer pair to an index node.
 */
int indexnode_add_key_pointer(BPlusIndexNode *node, int key, int left_child, int right_child);

/**
 * @brief Finds the child block ID to descend to for a given key.
 */
int indexnode_find_child(const BPlusIndexNode *node, int key);

/**
 * @brief Gets whether this is a leaf node.
 */
int indexnode_is_leaf(const BPlusIndexNode *node);

#endif