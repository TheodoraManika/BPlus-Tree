#ifndef BP_DATANODE_H
#define BP_DATANODE_H

#include "record.h"
#include "bf.h"

/**
 * @brief Represents a leaf node in the B+ tree containing records.
 */
typedef struct {
  int num_records;                              /**< Number of records in this leaf */
  int next_leaf;                                /**< Block ID of next leaf node (-1 if last) */
  int prev_leaf;                                /**< Block ID of previous leaf node (-1 if first) */
  char data[BF_BLOCK_SIZE - 3 * sizeof(int)];  /**< Records data */
} BPlusDataNode;

#endif
