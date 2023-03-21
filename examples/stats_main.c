#include "hash_statistics.h"
#define FILE_NAME "data.db"
#define INDEX_NAME "index.db"

int main()
{
    BF_Init(LRU);
    if(HashStatistics(FILE_NAME) == -1)
    {
        printf("An error occured!\n");
        return -1;
    }
    if(HashStatistics(INDEX_NAME) == -1)
    {
        printf("An error occured!\n");
        return -1;
    }
    BF_Close();
    return 0;
}