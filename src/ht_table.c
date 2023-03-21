#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "record.h"

int HT_CreateFile(char *fileName,  int buckets){
  if(BF_CreateFile(fileName) != BF_OK) //check if a ht file already exists
    return -1;
  int filedesc;
  if(BF_OpenFile(fileName,&filedesc) != BF_OK) //open the file and get its filedescriptor
    return -1;
  BF_Block* Block;
  BF_Block_Init(&Block); //initialize the block variable
  if(BF_AllocateBlock(filedesc,Block) != BF_OK) //allocate a block to the db file
      return -1;
  void* Data = BF_Block_GetData(Block); //get a pointer to its data
  int* type = Data;
  *type = 1;
  HT_info* info = Data + sizeof(int); 
  info->filedesc = filedesc; //set the values to the metadata of the file
  info->buckets = buckets; //the file descriptor and the number of buckets
  for(int i = 0; i < buckets;i++)
    info->HashArray[i] = -1; //set all the hash array vals to -1
  BF_Block_SetDirty(Block); //dirty so the blocks contents are written to the file
  if(BF_UnpinBlock(Block) != BF_OK) //unpin it
    return -1;
  if(BF_CloseFile(filedesc) != BF_OK) //close the file so it is reopened with open
    return -1;
  return 0;
}

HT_info* HT_OpenFile(char *fileName){
  int filedesc;
  if(BF_OpenFile(fileName,&filedesc) != BF_OK) //open the file
    return NULL; //if it fails return null
  BF_Block* block;
  BF_Block_Init(&block); //initialize a block
  if(BF_GetBlock(filedesc,0,block) != BF_OK) //get the metadata of the file
    return NULL;
  void* Data = BF_Block_GetData(block);
  int* type = Data;
  if(*type != 1)
    return NULL;
  HT_info* info = Data + sizeof(int);
  info->filedesc =  filedesc;
  return info; //return the data
}


int HT_CloseFile( HT_info* ht_info ){
  BF_Block* Block;
  BF_Block_Init(&Block);
  if(BF_GetBlock(ht_info->filedesc,0,Block) != BF_OK)
    return -1;
  BF_Block_SetDirty(Block);
  if(BF_UnpinBlock(Block) != BF_OK)
    return -1;
  if(BF_CloseFile(ht_info->filedesc) != BF_OK)
    return -1;
  return 0;
}

int HT_InsertEntry(HT_info* ht_info, Record record){
  BF_Block* Block; //some variables to be used
  BF_Block* prevBlock;
  void* data; //a pointer for the blocks data
  BF_Block_Init(&Block);BF_Block_Init(&prevBlock); //initialize both blocks
  int bucket = Hash_function(ht_info,record.id); //get the bucket in which we"ll insert the entry
  HT_block_info* blockinfo;
  Record* rec; //some more useful variables
  int bytesleft = BF_BLOCK_SIZE - sizeof(HT_block_info); //the bytes offset for the metadata of the block
  int blocknum;
  if(ht_info->HashArray[bucket] == -1) //if the bucket is empty
  {
    if(BF_AllocateBlock(ht_info->filedesc,Block) != BF_OK) //allocate a new block
      return -1;
    data = BF_Block_GetData(Block); //get a pointer to its data
    blockinfo = data + bytesleft; //a pointer to its metadata
    rec = data; //and another pointer to its data
    memcpy(rec,&record,sizeof(record)); //copy the records to the block
    blockinfo->Bytesleft= bytesleft - sizeof(record); //update the bytes available the block has
    blockinfo->Records = 1; //update the number of records it holds
    blockinfo->NextBlock = -1; //there is no next block so set it to -1
    BF_Block_SetDirty(Block); //dirty so the block's contents are written to the disc
    if(BF_UnpinBlock(Block) != BF_OK) //unpin it
      return -1;
    if(BF_GetBlockCounter(ht_info->filedesc,&blocknum) != BF_OK) //get the number of blocks(since our block will be the last one inserted)
      return -1;
    ht_info->HashArray[bucket] = blocknum - 1; //set the hasharray to point at that block
    return blocknum - 1; //return the block number
  }
  /*If the hash array already has a block allocated*/
  blocknum = ht_info->HashArray[bucket]; //get the block from the hash array
  if(BF_GetBlock(ht_info->filedesc,blocknum,Block) != BF_OK)
    return -1;
  while(1) //if nextblock becomes -1 we have reached the final block of bucket
  {
    data = BF_Block_GetData(Block); //get its data
    blockinfo = data + bytesleft; //go to the metadata offset
    int nextblock = blockinfo->NextBlock; //get the next block
    if(nextblock == -1)
    {
      if(blockinfo->Bytesleft >= sizeof(record)) //if the current block has bytes left for a record
      {
        rec = data + blockinfo->Records*sizeof(record); //go the the first available byte
        memcpy(rec,&record,sizeof(record)); //copy the contents of the record
        blockinfo->Bytesleft -= sizeof(record); //update the bytes left in the block
        blockinfo->Records += 1; //update the number of records in the block
        BF_Block_SetDirty(Block); //dirty the block
        if(BF_UnpinBlock(Block) != BF_OK) //unpin it
          return -1;
        return blocknum; //return the number of the block in the file
      }
      if(BF_UnpinBlock(Block) != BF_OK) //unpin the current one
        return -1;
      break;
    }
    if(BF_UnpinBlock(Block) != BF_OK) //unpin the current one
      return -1;
    if(BF_GetBlock(ht_info->filedesc,nextblock,Block) != BF_OK) //get the next one
      return -1;
    blocknum = nextblock;
  }
  /*If the block in the hash array has no space for a record*/
  if(BF_AllocateBlock(ht_info->filedesc,Block) != BF_OK) //allocate a new block in the file
    return -1;
  data = BF_Block_GetData(Block); //get a pointer to its data
  blockinfo = data + bytesleft; //go to the metadata offset
  rec = data; //get a pointer to the start of the data of the block
  memcpy(rec,&record,sizeof(record)); //copy the records contents
  blockinfo->Bytesleft= bytesleft - sizeof(record); //update the metadata of the block
  blockinfo->Records = 1;
  blockinfo->NextBlock = -1; //no next block so we set it to -1
  BF_Block_SetDirty(Block); //dirty the block
  if(BF_UnpinBlock(Block) != BF_OK) //unpin it
    return -1;
  /*Now we want to set the nextblock of the previous block to point at this block*/
  if(BF_GetBlock(ht_info->filedesc,blocknum,Block) != BF_OK) //get the last block of the bucket
    return -1;
  data = BF_Block_GetData(Block); //get its data
  blockinfo = data + bytesleft; //go to its metadata
  /*if the while breaks we have found the block we want*/
  int blockcounter;
  if(BF_GetBlockCounter(ht_info->filedesc,&blockcounter) != BF_OK) //get the number of blocks in the database
    return -1;
  blockinfo->NextBlock = blockcounter - 1; //the BF_GetBlockCounter() returns the number of blocks including 0,so we have blockcounter-1 blocks total
  BF_Block_SetDirty(Block); //dirty the block
  if(BF_UnpinBlock(Block) != BF_OK) //unpin it
    return -1;
  return blockcounter - 1; //return the block number
}

int HT_GetAllEntries(HT_info* ht_info, void* value ){
  int* ID = value;
  int bucket = Hash_function(ht_info,*ID); //get the bucket of the record we want
  BF_Block* Block;
  BF_Block_Init(&Block); //initialize the block var
  if(BF_GetBlock(ht_info->filedesc,ht_info->HashArray[bucket],Block) != BF_OK) //get the first block in the bucket
    return -1;
  void* data = BF_Block_GetData(Block); //get its data
  int bytesleft = BF_BLOCK_SIZE - sizeof(HT_block_info); //set the metadata offset
  HT_block_info* blockinfo = data + bytesleft; //get the metadata of the block
  Record* rec;
  int counter = 0;
  while(1) //nextblock will become -1 when we dont have another block in the the bucket
  {  
    counter++; //move to the next block  
    for(int i = 0;i < blockinfo->Records;i++) //check the records of the block
    {
      rec = data + i*sizeof(Record); //go the each record
      if(rec->id == *ID) //if the ID matches the key
      {
        printRecord(*rec); //print it
        //return counter; //return blocks parsed
      }
    }
    int nextblock = blockinfo->NextBlock;
    if(BF_UnpinBlock(Block) != BF_OK) //unpin the current one
      return -1;
    if(nextblock == -1) //if there is no next
      break; //break
    if(BF_GetBlock(ht_info->filedesc,nextblock,Block) != BF_OK) //get the next block
      return -1;
    data = BF_Block_GetData(Block); //get its data
    blockinfo = data + bytesleft; //get its metadata
  }
  return counter; //return blocks parsed
}

int Hash_function(HT_info* info,int ID){return (ID % info->buckets);}




