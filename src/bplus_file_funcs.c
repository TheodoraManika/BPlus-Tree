#include "bplus_file_funcs.h"
#include "bplus_datanode.h"
#include "bplus_index_node.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return bplus_ERROR;     \
    }                         \
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
    meta.root_block = 1;      // the root will be the block 1 
    meta.depth = 1;           // only leaf level 
    meta.file_desc = fd;   // runtime

    memcpy(data, &meta, sizeof(BPlusMeta));

    BF_Block_SetDirty(meta_block);
    CALL_BF(BF_UnpinBlock(meta_block));
    BF_Block_Destroy(&meta_block);

    // Block 1: root data node
    BF_Block *root_block;
    BF_Block_Init(&root_block);

    CALL_BF(BF_AllocateBlock(fd, root_block));

    BPlusDataNode node;
    datanode_init(&node); // sets num_records=0, next=-1, prev=-1

    char *root_data = BF_Block_GetData(root_block);
    memcpy(root_data, &node, sizeof(BPlusDataNode));

    BF_Block_SetDirty(root_block);
    CALL_BF(BF_UnpinBlock(root_block));
    BF_Block_Destroy(&root_block);

    CALL_BF(BF_CloseFile(fd));
  return -1;
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

  return -1;
}

int bplus_close_file(const int file_desc, BPlusMeta* metadata)
{
  free(metadata);
    CALL_BF(BF_CloseFile(file_desc));
  return -1;
}

// Το see if we want to do it as a team
int bplus_record_insert(const int file_desc, BPlusMeta *metadata, const Record *record)
{
  return -1;
}

int bplus_record_find(const int file_desc, const BPlusMeta *metadata, const int key, Record** out_record)
{ 
  int block_id = metadata->root_block;

    BF_Block *block;
    BF_Block_Init(&block);

    CALL_BF(BF_GetBlock(file_desc, block_id, block));
    char *data = BF_Block_GetData(block);

    BPlusDataNode *node = (BPlusDataNode*)data;

    int num = datanode_get_num_records(node);

    Record *tmp = malloc(sizeof(Record));

    for (int i = 0; i < num; i++) {
        datanode_get_record(node, i, &metadata->schema, tmp);

        if (record_get_key(&metadata->schema, tmp) == key) {
            *out_record = tmp;
            CALL_BF(BF_UnpinBlock(block));
            BF_Block_Destroy(&block);
            return bplus_OK;
        }
    }
    // if we don’t find it 
    free(tmp); 

    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);
  *out_record=NULL;
  return -1;
}


