#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

int HP_CreateFile(char *fileName){
  if(BF_CreateFile(fileName) != BF_OK) //create a new heap file
    return -1; //if it already exists return error
  int filedesc;
  if(BF_OpenFile(fileName,&filedesc) != BF_OK) //try to open the file
    return -1; //if it fails return an error
  BF_Block *Block;
  BF_Block_Init(&Block); //initialize a block
  if(BF_AllocateBlock(filedesc,Block) != BF_OK) //allocate enough space for it in the heap file
   return -1;
  void* data = BF_Block_GetData(Block) ; //get a pointer to the data of the block
  int* temp = data;
  *temp = 0;
  HP_info* info = data + sizeof(int); //set a pointer to the beginning of the data
  info->FileDesc = filedesc; //set apropriate values
  info->TotalRecords = 0;
  int bytesmove = BF_BLOCK_SIZE - sizeof(HP_block_info); //create the bytes offset
  BF_Block_SetDirty(Block); //write block to disc
  if(BF_UnpinBlock(Block)!= BF_OK) //unpin it
    return -1;
  if(BF_CloseFile(filedesc) != BF_OK) //close the file(user needs to use openfile to open it)
    return -1;
  return 0;
}

HP_info* HP_OpenFile(char *fileName){
  int filedesc;
  BF_Block* Block;
  BF_Block_Init(&Block); //initialize the block
  if(BF_OpenFile(fileName,&filedesc) != BF_OK) //try to open the file
    return NULL;
  if(BF_GetBlock(filedesc,0,Block) != BF_OK) //get block 0 which has the metadata of the heap file
    return NULL;
  void* data = BF_Block_GetData(Block); //get a pointer to the data
  int* type = data;
  if(*type !=  0)
    return NULL;
  HP_info* info = data + sizeof(int); //the metadata is at the start of the data
  info->FileDesc = filedesc;
  return info; //return metadata
}


int HP_CloseFile( HP_info* hp_info ){
  BF_Block* Block;
  BF_Block_Init(&Block);
  if(BF_GetBlock(hp_info->FileDesc,0,Block) != BF_OK)
    return -1;
  BF_Block_SetDirty(Block);
  if(BF_UnpinBlock(Block) != BF_OK)
    return -1;
  if(BF_CloseFile(hp_info->FileDesc) != BF_OK) //try to close the file
    return -1;
  return 0;
}

int HP_InsertEntry(HP_info* hp_info, Record record){
  int blocknum;
  if(BF_GetBlockCounter(hp_info->FileDesc,&blocknum) != BF_OK) //get the number of blocks in the heap file
    return -1;
  BF_Block* block;
  BF_Block_Init(&block);//initialize our Block entity
  void* data;
  HP_block_info* blockinfo;
  int bytesleft = BF_BLOCK_SIZE - sizeof(HP_block_info); //set the file offset
  if(blocknum > 1) //if we have more than the block0(which is not used to store records)
  {
    if(BF_GetBlock(hp_info->FileDesc,blocknum-1,block) != BF_OK) //get the last available block
      return -1;
    data = BF_Block_GetData(block); //get the block's data
    blockinfo = data + bytesleft; //set the file offset
    if(blockinfo->BytesLeft > sizeof(record)) //if we have space for at least one more record
    {
      Record* rec = data + (blockinfo->Records)*sizeof(record); //add a pointer pointing to the next available byte
      memcpy(rec,&record,sizeof(record)); //copy our record to the block
      blockinfo->BytesLeft -= sizeof(record); //update our available bytes
      blockinfo->Records+=1; //inform that we have one more record
      BF_Block_SetDirty(block); //write block to disc
      if(BF_UnpinBlock(block) != BF_OK) //unpin it
        return -1;
      return blocknum - 1; //return blocknum since we didnt create a new block
    }
  }//else if we dont have space in our last block or this is block 1
  if(BF_AllocateBlock(hp_info->FileDesc,block) != BF_OK) //allocate space for a new block
    return -1;
  data = BF_Block_GetData(block); //get a pointer to the start of its data space
  blockinfo = data + bytesleft; //add the offset so we can input the block's info to the last 16 bytes of the dataspace
  Record* rec = data; //add a pointer to the start of our dataspace
  memcpy(rec,&record,sizeof(record)); //copy the record to our block
  blockinfo->BytesLeft = bytesleft - sizeof(record); //update our available bytes
  blockinfo->Records = 1; //update our current records
  BF_Block_SetDirty(block); //set the block dirty so we write it to the disc
  if(BF_UnpinBlock(block) != BF_OK) //unpin the block
    return -1;
  return blocknum;//return blocknum + 1 since we created a new block 
}

int HP_GetAllEntries(HP_info* hp_info, int value){
  int blocknum;
  int found = 0;
  if(BF_GetBlockCounter(hp_info->FileDesc,&blocknum) != BF_OK) //get our current blocks
    return -1;
  if(blocknum <= 1) //if it at least one return error(we dont have records in block 0)
    return -1;
  BF_Block* block;
  BF_Block_Init(&block); //initialize a block
  int bytesmove = BF_BLOCK_SIZE - sizeof(HP_block_info),blockcounter = 0; //create the byte offset
  void* data;HP_block_info* info;Record* rec;
  for(int i = 1;i < blocknum;i++) //check all blocks
  {//since in heap files ,records are put in randomly
    if(BF_GetBlock(hp_info->FileDesc,i,block) != BF_OK) //get the a random block
      return -1;
    data = BF_Block_GetData(block); //get a pointer to its data
    info = data + bytesmove; //go to the file offset
    for(int j = 0;j < info->Records;j++) //for all its records
    {
      rec = data + j*sizeof(Record); //set the rec pointer to the next record
      if(rec->id == value) //if the record has the desired ID
      {
        printRecord(*rec);
        if(BF_UnpinBlock(block) != BF_OK) //unpin block
          return -1;
        blockcounter++; //update the value of our counter since it wont be updated in the loop
        return blockcounter; //cause the functio  will return here
      }

    }
    blockcounter++; //hold all the blocks parsed
    if(BF_UnpinBlock(block) != BF_OK) //unpin block
      return -1;
  }
  return -1;
}

