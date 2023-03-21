#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "bf.h"
#include "ht_table.h"
#include "sht_table.h"

#define RECORDS_NUM 200 // you can change it if you want
#define FILE_NAME "data.db"
#define INDEX_NAME "index.db"

int main() {
    srand(time(NULL));
    if(BF_Init(LRU) != BF_OK)
    {
      printf("BF level initialization failed!\n");
      return -1;
    }
    // Αρχικοποιήσεις
    bool checkpointHT[5];
    bool checkpointSHT[5];
    for(int i = 0;i < 5;i++)
    {
      checkpointHT[i] = true;
      checkpointSHT[i] = true;
    }
    if(HT_CreateFile(FILE_NAME,10) == -1)
      checkpointHT[0] = false;
    if(SHT_CreateSecondaryIndex(INDEX_NAME,10,FILE_NAME) == -1)
      checkpointSHT[0] = false;
    HT_info* info = HT_OpenFile(FILE_NAME);
    if(info == NULL)
      checkpointHT[1] = false;
    SHT_info* index_info = SHT_OpenSecondaryIndex(INDEX_NAME);
    if(index_info == NULL)
      checkpointSHT[1] = false;
    // Θα ψάξουμε στην συνέχεια το όνομα searchName
    Record record=randomRecord();
    char searchName[15];
    strcpy(searchName, record.name);
    // Κάνουμε εισαγωγή τυχαίων εγγραφών τόσο στο αρχείο κατακερματισμού τις οποίες προσθέτουμε και στο δευτερεύον ευρετήριο
    printf("Insert Entries\n");
    int check = 0;
    for (int id = 0; id < RECORDS_NUM; ++id) {
        record = randomRecord();
        int block_id = HT_InsertEntry(info, record);
        if(block_id == -1)
          checkpointHT[2] = false;
        check = SHT_SecondaryInsertEntry(index_info, record, block_id);
        if(check == -1)
          checkpointSHT[2] = false;
    }
    // Τυπώνουμε όλες τις εγγραφές με όνομα searchName
    printf("Printing all entries using the Secondary Hash Table,with the name : %s\n",searchName);
    int counter = SHT_SecondaryGetAllEntries(info,index_info,searchName);
    if(counter == -1)
      checkpointSHT[3] = false;
    else
      printf("Blocks parsed to find all the entries : %d\n",counter);
    // Κλείνουμε το αρχείο κατακερματισμού και το δευτερεύον ευρετήριο
    if(SHT_CloseSecondaryIndex(index_info) == -1)
     checkpointSHT[4] = false;
    if(HT_CloseFile(info) == -1)
      checkpointHT[4] = false;
    printf("\nPrinting statistics for the current main function : \n");
    int HTcounter = 0,SHTcounter = 0;
    for(int i = 0;i < 5;i++)
    {
      if(checkpointHT[i] == true)
        HTcounter++;
      if(checkpointSHT[i] == true)
        SHTcounter++;
    }
    if(HTcounter == 5)
      printf("Everything worked correctly from the Primary Hash File!\n");
    if(SHTcounter == 5)
      printf("Everything worked correctly from the Secondary Hash File!\n");
    if(checkpointHT[0] == false)
      printf("Failed to create the 'data.db' file which may have caused other functions to fail aswell.\n");
    if(checkpointHT[1] == false)
      printf("Failed to open 'data.db' file.\n");
    if(checkpointHT[2] == false)
      printf("Failed to insert atleast one entry to the Primary Hash File.\n");
    if(checkpointHT[3] == false)
      printf("Failed to find desired ID from the Primary Hash File.\n This may be due to invalid ID input.\n");
    if(checkpointHT[4] == false)
      printf("Failed to close Primary Hash File\nIf The HT_Create() or HT_Open() failed,its expected to happen.\n");
    printf("\nTotal score for the Primary Hash File : %d\n\n",HTcounter);
    if(checkpointSHT[0] == false)
      printf("Failed to create the 'index.db' file which may have caused other functions to fail aswell.\n");
    if(checkpointSHT[1] == false)
      printf("Failed to open 'index.db' file.\n");
    if(checkpointSHT[2] == false)
     printf("Failed to insert atleast one entry to the Secondary Hash File.\n");
    if(checkpointSHT[3] == false)
      printf("Failed to find desired name from the Secondary Hash File.\n This may be due to invalid name input.\n");
    if(checkpointSHT[4] == false)
      printf("Failed to close Secondary Hash File\nIf The SHT_Create() or SHT_Open() failed,its expected to happen.\n");
    printf("\nTotal Score for the Secondary Hash File : %d\n",SHTcounter);
    if(BF_Close() != BF_OK)
    {
      printf("Error closing BF level!\n");
      return -1;
    }
  return 0;
}
