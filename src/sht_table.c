#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "sht_table.h"
#include "ht_table.h"
#include "record.h"

int SHT_CreateSecondaryIndex(char *sfileName,  int buckets, char* fileName){
  if(BF_CreateFile(sfileName) != BF_OK) //Create a new file
    return -1;
  int filedesc;
  if(BF_OpenFile(sfileName,&filedesc) != BF_OK) //open it and get the file descriptor pointing to it
    return -1;
  BF_Block* Block;
  BF_Block_Init(&Block); //initialize a block
  if(BF_AllocateBlock(filedesc,Block) != BF_OK) //allocate one to pass our metadata
    return -1;
  void* Data = BF_Block_GetData(Block); //get a pointer to its data
  int* type = Data;
  *type = 2;
  SHT_info* info = Data + sizeof(int);
  info->buckets = buckets; //set the correct values
  info->filedesc = filedesc;
  for(int i = 0;i < buckets;i++)
    info->HashArray[i] = -1; //initialize each value in the array as -1 (its helpful in the insert entry section)
  BF_Block_SetDirty(Block); //write block to disc
  if(BF_UnpinBlock(Block) != BF_OK) //unpin it
    return -1;
  if(BF_CloseFile(filedesc) != BF_OK) //close the file
    return -1;
  return 0;
}

SHT_info* SHT_OpenSecondaryIndex(char *indexName){
  int filedesc;
  if(BF_OpenFile(indexName,&filedesc) != BF_OK) //open the file
    return NULL;
  BF_Block* Block;
  BF_Block_Init(&Block); //initialize a block
  if(BF_GetBlock(filedesc,0,Block) != BF_OK) //get the block0 (since it has the metadata)
    return NULL;
  void* Data = BF_Block_GetData(Block);
  int* type = Data;
  if(*type != 2)
    return NULL;
  SHT_info* info = Data + sizeof(int);
  info->filedesc = filedesc;
  return info; //return the metadata
}


int SHT_CloseSecondaryIndex( SHT_info* SHT_info ){
  BF_Block* Block;
  BF_Block_Init(&Block);
  if(BF_GetBlock(SHT_info->filedesc,0,Block) != BF_OK)
    return -1;
  BF_Block_SetDirty(Block);
  if(BF_UnpinBlock(Block) != BF_OK)
    return -1;
  if(BF_CloseFile(SHT_info->filedesc) != BF_OK) //close the file
    return -1;
  return 0;
}

int SHT_SecondaryInsertEntry(SHT_info* sht_info, Record record, int block_id){
  /*Variables and block initialization*/
  BF_Block* Block;BF_Block *prevBlock;
  void* Data;
  BF_Block_Init(&Block);BF_Block_Init(&prevBlock);
  SHT_block_info* blockinfo;
  sRecord* rec;sRecord rec1;//The sRecord struct holds a char* name and a BlockID integer
  int blocknum;int bytesleft = BF_BLOCK_SIZE - sizeof(SHT_block_info);
  int bucket = hash(sht_info,record.name);//get the bucket holding the record
  /*Assign proper values to the sRecord variable*/
  rec1.BlockID = block_id;
  strcpy(rec1.name,record.name);
  if(sht_info->HashArray[bucket] == -1) //if the array val is -1 there is no block in the bucket
  {
    if(BF_AllocateBlock(sht_info->filedesc,Block) != BF_OK) //allocate a new one
      return -1;
    Data = BF_Block_GetData(Block); //get a pointer to its data
    blockinfo = Data + bytesleft; //get a pointer to its metadata
    rec = Data; //get a SRecord pointer to its data
    memcpy(rec,&rec1,sizeof(rec1)); //copy the contents of rec1 to the block
    /*Update the block's metadata*/
    blockinfo->Bytesleft = bytesleft - sizeof(rec1);
    blockinfo->Records = 1;
    blockinfo->NextBlock = -1; //set the nextblock to -1 so we know this is the last block of the bucket (see line 108)
    BF_Block_SetDirty(Block);//write block to disc
    if(BF_UnpinBlock(Block) != BF_OK) //unpin it
      return -1;
    if(BF_GetBlockCounter(sht_info->filedesc,&blocknum) != BF_OK) //get the number of blocks in the file
      return -1;
    sht_info->HashArray[bucket] = blocknum - 1; //the number of our block is the total number of blocks-1
    return blocknum-1; //return the block we wrote into
  }
  /*if the above if doesn't return,there is at least a block in the bucket*/
  blocknum = sht_info->HashArray[bucket]; //get that blockID
  if(BF_GetBlock(sht_info->filedesc,blocknum,Block) != BF_OK) //get the block
    return -1;
  /*Pointers to its data*/
  while(1) //An infinite loop to find the last block of the bucket
  {
    Data = BF_Block_GetData(Block);
    blockinfo = Data + bytesleft;
    int NextBlock = blockinfo->NextBlock; //get the next block
    if(NextBlock == -1) //if there is no next block (we are at the last block of the bucket)
    {
      if(blockinfo->Bytesleft >= sizeof(rec1)) //check if it has available bytes for the record
      {
        rec = Data + blockinfo->Records*sizeof(sRecord); //go the first available byte in the block
        memcpy(rec,&rec1,sizeof(rec1)); //copy the name and blockid to the block
        /*Update the metadata*/
        blockinfo->Bytesleft -= sizeof(rec1);
        blockinfo->Records += 1;
        BF_Block_SetDirty(Block); //write block to disc
        if(BF_UnpinBlock(Block) != BF_OK) //unpin the block
          return -1;
        return blocknum; //retutn the blockID of the block we wrote into
      
      }
      /*If there is no next block but the above if statement fails*/
      /*We need to create a new block,which will happen out of the while loop*/
      if(BF_UnpinBlock(Block) != BF_OK) //unpin the block
        return -1;
      break; //break the loop
    }
    /*if there is another block after this one*/
    if(BF_UnpinBlock(Block) != BF_OK) //unpin the current one
      return -1;
    if(BF_GetBlock(sht_info->filedesc,NextBlock,Block) != BF_OK) //get the next one
      return -1;
    blocknum = NextBlock; //set blocknum to the nextblock val(the last block we have,because on the next iteration of the while loop the nextblock val will change)
  }
  /*If we reach this point we need to allocate a new block to the end of the bucket*/
  if(BF_AllocateBlock(sht_info->filedesc,Block) != BF_OK) //allocate a new block
    return -1;
  /*Get pointers to its data*/
  Data = BF_Block_GetData(Block); 
  rec = Data;
  memcpy(rec,&rec1,sizeof(rec1)); //copy record to the block
  /*Set metadata pointer to the end of the block*/
  blockinfo = Data + bytesleft;
  /*Update the block's metadata*/
  blockinfo->Bytesleft = bytesleft - sizeof(rec1);
  blockinfo->NextBlock = -1;
  blockinfo->Records = 1;
  BF_Block_SetDirty(Block); //write block to disc
  if(BF_UnpinBlock(Block) != BF_OK) //unpin it
    return -1;
  /*Now we will find the previous block (of the one above) to point it to the last one*/
  if(BF_GetBlock(sht_info->filedesc,blocknum,Block) != BF_OK) //get the last block of the bucket
    return -1;
  Data = BF_Block_GetData(Block); //get its data
  blockinfo = Data + bytesleft; //and its metadata
  int blockcounter;//get the number of blocks in the file
  if(BF_GetBlockCounter(sht_info->filedesc,&blockcounter) != BF_OK)
    return -1;
  blockinfo->NextBlock = blockcounter - 1; //set the previous block to point at the last one
  BF_Block_SetDirty(Block); //write block to disc
  if(BF_UnpinBlock(Block) != BF_OK) //unpin it
    return -1;
  return blockcounter - 1; //return the blockID
}

int SHT_SecondaryGetAllEntries(HT_info* ht_info, SHT_info* sht_info, char* name){
  int bucket = hash(sht_info,name); //get the bucket in which the entry is
  /*Initialize variables and Blocks*/
  BF_Block* Block;
  BF_Block* Block2;
  BF_Block_Init(&Block);
  BF_Block_Init(&Block2);
  void* Data;
  SHT_block_info* blockinfo;
  sRecord* rec1;
  int counter = 0,bytesleft = BF_BLOCK_SIZE - sizeof(SHT_block_info);
  /*Get the first block in the bucket*/
  if(BF_GetBlock(sht_info->filedesc,sht_info->HashArray[bucket],Block) != BF_OK)
    return -1;
  /*Pointers to its data*/
  Data = BF_Block_GetData(Block);
  blockinfo = Data + bytesleft;
  while (1)
  {
    counter++; //increment the counter since we search a new block
    for(int i = 0;i < blockinfo->Records;i++)
    {
      rec1 = Data + i*sizeof(sRecord); //search each record 
      if(strcmp(rec1->name,name) == 0) //if the name of the record is the same with the one we want
      {
        if(BF_GetBlock(ht_info->filedesc,rec1->BlockID,Block2) != BF_OK) //get the block from the main hash file
          return -1;
        void* Data2 = BF_Block_GetData(Block2); //get its data
        HT_block_info* ht_block_info = Data2 + (BF_BLOCK_SIZE - sizeof(HT_block_info)); //and its metadata
        for(int i = 0;i < ht_block_info->Records;i++)
        {
          Record* rec = Data2 + i*sizeof(Record); //check each record in the block
          if(strcmp(rec->name,rec1->name) == 0) //if there is the record we want
            printRecord(*rec); //print it
        }
        if(BF_UnpinBlock(Block2) != BF_OK) //unpin the block
          return -1;
      }
      /*The loop checks each record in the sht file since we may have multiple times the same name*/
    }
    int nextblock = blockinfo->NextBlock; //get the next block
    if(BF_UnpinBlock(Block) != BF_OK) //unpin the current one
      return -1;
    if(nextblock == -1) //if there is no next block
      break; //break the loop
    if(BF_GetBlock(sht_info->filedesc,nextblock,Block) != BF_OK) //get the next block
      return -1;
    /*Pointers to its data*/
    Data = BF_Block_GetData(Block);
    blockinfo = Data + bytesleft;
    /*This loop checks all blocks in the bucket,since we may have the same name written multiple times and we can't be sure as to in which block each time the name is written*/
  }
  /*retun the number of blocks parsed*/
  return counter;
}

unsigned int hash(SHT_info* sht_info,char* name)
{
  int length = strlen(name);
  unsigned int hash_value = 0;
  for(int i = 0; i < length; i++) //for each char in the string
  {
    hash_value += name[i]; //add its ASCII value to the hash val
    hash_value = (hash_value * name[i]) % sht_info->buckets;//make it in the range of 1-num of Buckets
  }
  return hash_value;
}


