#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "ht_table.h"
#include "sht_table.h"

int HashStatistics(char* filename);
int HTStatistics(HT_info* info);
int SHTStatistics(SHT_info* info);