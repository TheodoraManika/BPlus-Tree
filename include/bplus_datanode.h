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

/**
 * @brief Initializes an empty data node.
 */
void datanode_init(BPlusDataNode *node);

/**
 * @brief Gets the number of records in a data node.
 */
int datanode_get_num_records(const BPlusDataNode *node);

/**
 * @brief Gets a record from a data node at a specific position.
 */
int datanode_get_record(const BPlusDataNode *node, int pos, const TableSchema *schema, Record *out_record);

/**
 * @brief Sets a record in a data node at a specific position.
 */
int datanode_set_record(BPlusDataNode *node, int pos, const Record *record, const TableSchema *schema);

/**
 * @brief Adds a record to a data node.
 */
int datanode_add_record(BPlusDataNode *node, const Record *record, const TableSchema *schema);

/**
 * @brief Gets the next leaf block ID.
 */
int datanode_get_next_leaf(const BPlusDataNode *node);

/**
 * @brief Sets the next leaf block ID.
 */
void datanode_set_next_leaf(BPlusDataNode *node, int next_leaf);

/**
 * @brief Gets the previous leaf block ID.
 */
int datanode_get_prev_leaf(const BPlusDataNode *node);

/**
 * @brief Sets the previous leaf block ID.
 */
void datanode_set_prev_leaf(BPlusDataNode *node, int prev_leaf);

#endif
