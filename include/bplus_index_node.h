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
} BPlusIndexNode;

#endif