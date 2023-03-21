#include "hash_statistics.h"

int HashStatistics(char* filename)
{
    int filedesc;
    if(BF_OpenFile(filename,&filedesc) != BF_OK)
        return -1;
    BF_Block* Block;
    BF_Block_Init(&Block);
    if(BF_GetBlock(filedesc,0,Block) != BF_OK)
        return -1;
    void* Data = BF_Block_GetData(Block);
    int* type = Data;
    if(*type == 1) //HT file
    {
        BF_CloseFile(filedesc);
        if(BF_UnpinBlock(Block) != BF_OK)
            return -1;
        HT_info* info = HT_OpenFile(filename);
        if(info == NULL)
            return -1;
        int check = HTStatistics(info);
        HT_CloseFile(info);
        if(check == 0)
            return 0;
        return -1;
    }
    else if(*type == 2) //SHT file
    {
        BF_CloseFile(filedesc);
        if(BF_UnpinBlock(Block) != BF_OK)
            return -1;
        SHT_info* info = SHT_OpenSecondaryIndex(filename);
        if(info == NULL)
            return -1;
        int check = SHTStatistics(info);
        SHT_CloseSecondaryIndex(info);
        if(check == 0)
            return 0;
        return -1;
    }
    printf("An error occured!\nYou either used an Heap file as an input or a wrong filename.\n");
    if(BF_CloseFile(filedesc) != BF_OK)
        return -1;
    return -1;
}

int HTStatistics(HT_info* info)
{
    int Counter;
    if(BF_GetBlockCounter(info->filedesc,&Counter) != BF_OK)
        return -1;
    BF_Block* Block;
    BF_Block_Init(&Block);
    void* Data;
    HT_block_info* BlockInfo;
    int byteoffset = BF_BLOCK_SIZE - sizeof(HT_block_info);
    int RecordCounter = 0;
    int LeastRecords = 6;
    int MaxRecords = 0;
    int Blocks_Per_Bucket[info->buckets];
    for(int i = 0;i < info->buckets;i++)
        Blocks_Per_Bucket[i] = 0;
    printf("Total Blocks of the Primary Hash File are: %d\n",Counter);
    for(int i = 0;i < info->buckets;i++)
    {
        int NextBlock = info->HashArray[i];
        while(1)
        {
            if(BF_GetBlock(info->filedesc,NextBlock,Block) != BF_OK)
                return -1;
            Blocks_Per_Bucket[i]+=1;
            Data = BF_Block_GetData(Block);
            BlockInfo = Data + byteoffset;
            if(LeastRecords >= BlockInfo->Records)
                LeastRecords = BlockInfo->Records;
            if(MaxRecords <= BlockInfo->Records)
                MaxRecords = BlockInfo->Records;
            RecordCounter += BlockInfo->Records;
            NextBlock = BlockInfo->NextBlock;
            if(BF_UnpinBlock(Block) != BF_OK)
                return -1;
            if(NextBlock == -1)
                break;               
        }
        printf("\n");
        printf("Printing Statistics for Bucket %d of Primary Hash Function\n",i);
        printf("The least records in a block are : %d\n",LeastRecords);
        printf("The max records in a block are : %d\n",MaxRecords);
        printf("The average number of records in a block are : %d\n",RecordCounter / Blocks_Per_Bucket[i]);
        LeastRecords = 6;
        MaxRecords = 0;
        RecordCounter = 0;
    }
    printf("\n");
    printf("Average number of blocks per bucket are : %d\n", (Counter - 1) / info->buckets);
    printf("\n");
    return 0;
}

int SHTStatistics(SHT_info* info)
{
    int Counter;
    if(BF_GetBlockCounter(info->filedesc,&Counter) != BF_OK)
        return -1;
    BF_Block* Block;
    BF_Block_Init(&Block);
    void* Data;
    SHT_block_info* BlockInfo;
    int byteoffset = BF_BLOCK_SIZE - sizeof(SHT_block_info);
    int RecordCounter = 0;
    int LeastRecords = 25;
    int MaxRecords = 0;
    int Blocks_Per_Bucket[info->buckets];
    for(int i = 0;i < info->buckets;i++)
        Blocks_Per_Bucket[i] = 0;
    printf("Total Blocks of the Secondary Hash File are: %d\n",Counter);
    for(int i = 0;i < info->buckets;i++)
    {
        int NextBlock = info->HashArray[i];
        if(NextBlock != -1)
        {
            while(1)
            {
                if(BF_GetBlock(info->filedesc,NextBlock,Block) != BF_OK)
                    return -1;
                Blocks_Per_Bucket[i]+=1;
                Data = BF_Block_GetData(Block);
                BlockInfo = Data + byteoffset;
                if(LeastRecords >= BlockInfo->Records)
                    LeastRecords = BlockInfo->Records;
                if(MaxRecords <= BlockInfo->Records)
                    MaxRecords = BlockInfo->Records;
                RecordCounter += BlockInfo->Records;
                NextBlock = BlockInfo->NextBlock;
                if(BF_UnpinBlock(Block) != BF_OK)
                    return -1;
                if(NextBlock == -1)
                    break;               
            }
            printf("\n");
            printf("Printing Statistics for Bucket %d of Secondary Hash Function\n",i);
            printf("The least records in a block are : %d\n",LeastRecords);
            printf("The max records in a block are : %d\n",MaxRecords);
            printf("The average number of records in a block are : %d\n",RecordCounter / Blocks_Per_Bucket[i]);
            LeastRecords = 6;
            MaxRecords = 0;
            RecordCounter = 0;
            }
    }
    printf("\n");
    printf("Average number of blocks per bucket are : %d\n", (Counter - 1) / info->buckets); 
    return 0;
}