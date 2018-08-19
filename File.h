#ifndef __FILEOPERATE_H
#define __FILEOPERATE_H

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <semaphore.h>

#define MAXREADER 1
#define NUM 30

//目录项结构：
struct dirUnit{
    char fileName[NUM]; //文件名
    char type;          //文件类型,0为目录,1为文件
    int startBlock;     //FCB起始盘块
};



//一个目录项包含了文件名和文件类型，当文件为目录时，起始盘块指示了目录表所在的盘块号，当文件为文件时，起始盘块指示了FCB所在的盘块号。
#define DIRTABLE_MAX_SIZE 15    //目录表项最大值
//目录表结构：
struct dirTable {
    int dirUnitAmount;//目录项数目
    dirUnit dirs[DIRTABLE_MAX_SIZE];//目录项列表
};
/*
本系统规定一个目录表只占用一个盘块，一个目录项大小为64B，所以一个目录表中最多可含15个目录项，dirUnitAmount记录每个目录表中已含有的目录项数目。系统在初始化时，会自动生成一个空的根目录表存放于磁盘中，作为用户的初始位置，用户所有的目录和文件都这个表为根进行树状目录结构的展开。
当创建一个目录表时，系统会自动为目录表加上一项名为”..”的目录项，指示父目录表的位置。
*/

//文件控制块结构：
struct FCB {
    int blockNum;   //文件数据起始盘块号
    int fileSize;   //文件大小，盘块为单位
    int dataSize;   //已写入的内容大小，字节为单位
    int readptr;    //读指针，字节为单位
    int link;       //文件链接数


    /* 信号量 */
    sem_t *count_sem;
    sem_t *write_sem;
};
/*
   文件控制块包含了文件数据的起始位置和大小。dataSize,readptr是为文件的读写操作而准备的，记录文件已写入的内容长度（不可超过文件大小），和当前读取的位置。Link记录了文件的链接数，用于文件的共享，当文件的链接数为0时，系统可以回收文件的空间。同样的，一个FCB大小为20B，但也用一个盘块保存。
   由于采用的是连续分配方式，所以系统规定文件被创建时，必须给出文件的大小，而且后期也不能修改文件的大小。
   */

void initRootDir();
char* getPath();
void showDir();
int changeDir(char dirName[]);
int changeName(char oldName[], char newName[]);
int creatFile(char fileName[], int fileSize);
int creatDir(char dirName[]);
int creatFCB(int fcbBlockNum, int fileBlockNum, int fileSize);
int addDirUnit(dirTable* myDirTable, char fileName[], int type, int FCBBlockNum);
int deleteFile(char fileName[]);
int releaseFile(int FCBBlock);
int deleteDirUnit(dirTable* myDirTable, int unitIndex);
int deleteDir(char dirName[]);
int deleteFileInTable(dirTable* myDirTable, int unitIndex);
int my_read(char fileName[], int length);
int my_write(char fileName[], char content[]);
int findUnitInTable(dirTable* myDirTable, char unitName[]);

#endif
