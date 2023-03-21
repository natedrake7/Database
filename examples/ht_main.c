#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "bf.h"
#include "ht_table.h"

#define RECORDS_NUM 400 // you can change it if you want
#define FILE_NAME "data.db"

int main() {
  srand(time(NULL));
  if(BF_Init(LRU) != BF_OK)
  {
    printf("BF level initialization failed!\n");
    return -1;
  }
  bool checkpoint[5];
  for(int i = 0;i < 5;i++)
    checkpoint[i] = true;
  if(HT_CreateFile(FILE_NAME,10) == -1)
    checkpoint[0] = false;
  HT_info* info = HT_OpenFile(FILE_NAME);
  if(info == NULL)
    checkpoint[1] = false;
  Record record;
  printf("Insert Entries\n");
  for (int id = 0; id < RECORDS_NUM; ++id) {
    record = randomRecord();
    if(HT_InsertEntry(info, record) == -1)
      checkpoint[2] = false;
  }
  int id = rand() % RECORDS_NUM;
  printf("Printing all entries with ID : %d\n",id);
  int counter = HT_GetAllEntries(info, &id);
  if(counter == -1)
    checkpoint[3] = false;
  else
    printf("Blocks parsed to find all the desired entries : %d\n",counter);
  if(HT_CloseFile(info) == -1)
    checkpoint[4] = false;
  counter = 0;
  for(int i = 0;i < 5;i++)
  {
    if(checkpoint[i] == true)
      counter++;
  }
  if(counter == 5)
    printf("Everything worked correctly with the Hash File!\n");
  
  if(checkpoint[0] == false)
    printf("Failed to create the 'data.db' file which may have caused other functions to fail aswell.\n");
  if(checkpoint[1] == false)
    printf("Failed to open 'data.db' file.\n");
  if(checkpoint[2] == false)
    printf("Failed to insert atleast one entry to the Hash File.\n");
  if(checkpoint[3] == false)
    printf("Failed to find desired ID from the Hash File.\n This may be due to invalid ID input.\n");
  if(checkpoint[4] == false)
    printf("Failed to close Hash File\nIf The HT_Create() or HT_Open() failed,its expected to happen.\n");
  printf("\nTotal score for the Hash File is : %d\n",counter);
  if(BF_Close() != BF_OK)
  {
    printf("Error closing BF level!\n");
    return -1;
  }

  return 0;
}
