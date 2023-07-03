Λαμπρόπουλος Κωνσταντίνος
Γκέργκη Δημήτρης


This project utilizes different methods of savind data ,and is used as a simulation of a database.In more detail,it uses the bf library to create and manage 3 different files:
HeapFile,StaticHashFile,SecondaryHashFile.
HeapFile is the simplest way to save data,where each data which is about to be saved,
is saved at the end of file.HashFile utilizes a hash function and breaks the file down to bueckts,and saves the data to the correspoding bucket(in the primary hash files,hashing is being done to the primary key).
Lastly,the SecondaryHashFile utilizes the name of a record(records are the data to be saved to the database) and hashes it ,and adds to the correspoding bucket the name and the blockID in which the whole record
resides in the primary Hash File.

There are detailed comments for the implementations in the *.c files.
The main functions have a way of grading the performance of each implementation(Heap,Primary Hash,Secondary Hash).
The Create(),Open(),Close() functions in all of the 3 implementations are the almost the same.

The Create() initializes the database file and saves the metadata to the first block(e.g. file descriptor,bucket size).
There is also an integer value at the first block,which is used to identify which file we have.If it is 0 we have a heap file,
if it is 1 Primary HashFile and if 2 SecondaryHashFile.The Create() also closes the filedescriptor correspoding to the file we opened(hence closes the file).

The Open() opens a file given a filename variable,gets the first block and checks the integer value ,to check if the correct function was used to open the file.
If all is done correctly ,it assigns to the variable filedesc(of the first block) the new filedescriptor given to the file from the BF level and returns the metadata of the file.
By using Open() the first block is pinned and needs to be unpinned when we want to close the file. 

The Close() unpins the first block the of the file we want to close,and then closes the file.

Heap File Implementations:


1)Insert Entries:
The entry is inserted to the end of the .db file.In order to do that,we check if the last block allocated to the file has enough space for a record,and if it does,we insert it there.
Differently we allocate a new block to the end of file and insert the new record there.

2)Get All Entries:
The records are saved randomly,so we use brute force search in the whole database.We check every block and every record,until we have the one we want.
If we find it we print the record and return the number of blocks parsed(There are not duplicate keys).

Primary Hash File Implementations: 

1)Insert Entry : 
The entry is now inserted to the end of the bucket,which corresponds to the record.To find the bucket,we hash the primary key and the value returned by the hash
corresponds to the bucket we will insert the record.We check by the metadata to find if the bucket already has blocks or not (using the HashArray).If it doesn't
we allocate a new block and insert the record we set to the bucket (the correspoding cell of the hash array with a pointer to the bucket ID) the ID of the block.
In any other case,we search for the last block of the bucket(the blocks in a bucket are a simple linked list).When we find it,we check if there is space for another record,
if not we allocate a new block and must connect the previous one with this one. 

2)Get All Entries :
Find the bucket where the hash function leads us,and search the whole bucket until we have the record.Print the record and return the number of blocks written(there are not duplicate keys).

Secondary Hash File Implementations:

1)The record insertion is identical to the primary hash file,with the only difference being that instead of the primary key being hashed,now the name of the record is hashed.

Note: The name and BlockID are being saved to the struct sRecord which resides in the sht_table.h.

2)Get All Entries :
The search is being done by the name and not the primary key.We use the hash function of the SHT to find the bucket in which the name resides.Here though,
we have to check all the blocks in the bucket,since names are expected to have duplicate values.The search is much faster though,since each block has 26 records,
and not 6 like in the Primary Hash File.

Compilation:
For the hp_main,ht_main,sht_main there is a makefile with the commands make hp,make ht,make sht.There is also a makefile command for the hash statistics function,which by using the commands make stats,you
can run the executable :./build/stats_main .
