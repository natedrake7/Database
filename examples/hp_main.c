#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "bf.h"
#include "hp_file.h"

#define RECORDS_NUM 100 // you can change it if you want
#define FILE_NAME "hp_data.db"

int main() {
  srand(time(NULL));
  if(BF_Init(LRU) != BF_OK)
  {
    printf("BF level initialization failed!\n");
    return -1;
  }
  bool checkpoint[5];
  for(int i = 0; i < 5;i++)
    checkpoint[i] = true;
  if(HP_CreateFile(FILE_NAME) == -1)
    checkpoint[0] = false;
  HP_info* info = HP_OpenFile(FILE_NAME);
  if(info == NULL)
    checkpoint[1] = false;
  Record record;
  printf("Insert Entries to the Heap file\n");
  for (int id = 0; id < RECORDS_NUM; ++id) {
    record = randomRecord();
    int val = HP_InsertEntry(info, record);
    if(val == -1)
      checkpoint[2] = false;
  }
  int id = rand() % RECORDS_NUM;
  printf("Printing all entries from the Heap file with ID : %d\n",id);
  int blocknum = HP_GetAllEntries(info, id);
  if(blocknum == -1)
    checkpoint[3] = false;
  else
    printf("Blocks parsed to reach the desired ID : %d\n",blocknum);
  if(HP_CloseFile(info) == -1)
    checkpoint[4] = false;
  int counter = 0;
  for(int i = 0;i  < 5;i++)
  {
    if(checkpoint[i] == true)
      counter++;
  }
  if(counter == 5)
    printf("Everything works correctly on the Heap File!\n");
  if(checkpoint[0] == false)
    printf("Failed to create the 'hp_data.db' file which may have caused other functions to fail aswell.\n");
  if(checkpoint[1] == false)
    printf("Failed to open 'hp_data.db' file.\n");
  if(checkpoint[2] == false)
    printf("Failed to insert atleast one entry to the Heap File.\n");
  if(checkpoint[3] == false)
    printf("Failed to find desired ID from the Heap File.\n This may be due to invalid ID input.\n");
  if(checkpoint[4] == false)
    printf("Failed to close Heap File\nIf The HP_Create() or HP_Open() failed,its expected to happen.\n");
  printf("\nTotal score for the Heap File is : %d\n",counter);
  if(BF_Close() != BF_OK)
  {
    printf("Error closing BF level!\n");
    return -1;
  }
  return 0;
}
