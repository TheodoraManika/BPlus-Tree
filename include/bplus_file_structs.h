//
// Created by theofilos on 11/4/25.
//

#ifndef BPLUS_BPLUS_FILE_STRUCTS_H
#define BPLUS_BPLUS_FILE_STRUCTS_H
#include "bf.h"
#include "bplus_datanode.h"
#include "bplus_index_node.h"
#include "record.h"
#include "bplus_file_structs.h"

typedef struct {
  int root_block;           /**< Block ID of the root */
  int height;               /**< Height of the B+ tree */
  int file_desc;            /**< File descriptor */
  TableSchema schema;       /**< Table schema */
} BPlusMeta;

#endif //BPLUS_BPLUS_FILE_STRUCTS_H

