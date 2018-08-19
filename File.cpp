#include "File.h"
#include "Disk.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define NUMREADER 5

dirTable* rootDirTable; //根目录
dirTable* currentDirTable;  //当前位置
char path[200]; //保存当前绝对路径


//初始化根目录
void initRootDir()
{
    //分配一个盘块空间给rootDirTable
    int startBlock = getBlock(1);
    if(startBlock == -1)
        return;
    rootDirTable = (dirTable*)getBlockAddr(startBlock);
    rootDirTable->dirUnitAmount = 0;
    //将自身作为父级目录
    //addDirUnit(rootDirTable, "..", 0, startBlock);

    currentDirTable = rootDirTable;
    //初始化初始绝对路径
    path[0]='/';
    path[1]='\0';
}


//获得绝对路径
char* getPath()
{
    return path;
}


//展示当前目录 ls
void showDir()
{
    int unitAmount = currentDirTable->dirUnitAmount;
    printf("total:%d\n", unitAmount);
    printf("name\ttype\tsize\tFCB\tdataStartBlock\n");
    //遍历所有表项
    for(int i=0; i<unitAmount; i++)
    {
        //获取目录项
        dirUnit unitTemp = currentDirTable->dirs[i];
        printf("%s\t%d\t", unitTemp.fileName, unitTemp.type);
        //该表项是文件，继续输出大小和起始盘块号
        if(unitTemp.type == 1)
        {
            int FCBBlock = unitTemp.startBlock;
            FCB* fileFCB = (FCB*)getBlockAddr(FCBBlock);
            printf("%d\t%d\t%d\n", fileFCB->fileSize, FCBBlock, fileFCB->blockNum);
        }else{
            int dirBlock = unitTemp.startBlock;
            dirTable* myTable = (dirTable*)getBlockAddr(dirBlock);
            printf("%d\t%d\n",myTable->dirUnitAmount, unitTemp.startBlock);
        }
    }
}


//切换目录 cd
int changeDir(char dirName[])
{
    //目录项在目录位置
    int unitIndex = findUnitInTable(currentDirTable, dirName);
    //不存在
    if(unitIndex == -1)
    {
        printf("file not found\n");
        return -1;
    }
    if(currentDirTable->dirs[unitIndex].type == 1)
    {
        printf("not a dir\n");
        return -1;
    }
    //修改当前目录
    int dirBlock = currentDirTable->dirs[unitIndex].startBlock;
    currentDirTable = (dirTable*)getBlockAddr(dirBlock);
    //修改全局绝对路径
    if(strcmp(dirName, "..") == 0)
    {
        //回退绝对路径
        int len = strlen(path);
        for(int i=len-2;i>=0;i--)
            if(path[i] == '/')
            {
                path[i+1]='\0';
                break;
            }
    }else {
        strcat(path, dirName);
        strcat(path, "/");
    }

    return 0;
}



//修改文件名或者目录名 mv
int changeName(char oldName[], char newName[])
{
    int unitIndex = findUnitInTable(currentDirTable, oldName);
    if(unitIndex == -1)
    {
        printf("file not found\n");
        return -1;
    }
    strcpy(currentDirTable->dirs[unitIndex].fileName, newName);
    return 0;
}


//******************创建和删除文件********************
//创建文件 touch
int creatFile(char fileName[], int fileSize)
{
    //检测文件名字长度
    if(strlen(fileName) >= NUM)
    {
        printf("file name too long\n");
        return -1;
    }
    //获得FCB的空间
    int FCBBlock = getBlock(1);
    if(FCBBlock == -1)
        return -1;
    //获取文件数据空间
    int FileBlock = getBlock(fileSize);
    if(FileBlock == -1)
        return -1;
    //创建FCB
    if(creatFCB(FCBBlock, FileBlock, fileSize) == -1)
        return -1;
    //添加到目录项
    if(addDirUnit(currentDirTable, fileName, 1, FCBBlock) == -1)
        return -1;

    return 0;
}


//创建目录 mkdir
int creatDir(char dirName[])
{
    if(strlen(dirName) >= NUM)
    {
        printf("file name too long\n");
        return -1;
    }
    //为目录表分配空间
    int dirBlock = getBlock(1);
    if(dirBlock == -1)
        return -1;
    //将目录作为目录项添加到当前目录
    if(addDirUnit(currentDirTable, dirName, 0, dirBlock) == -1)
        return -1;
    //为新建的目录添加一个到父目录的目录项
    dirTable* newTable = (dirTable*)getBlockAddr(dirBlock);
    newTable->dirUnitAmount = 0;
    char parent[] = "..";
    if(addDirUnit(newTable, parent, 0, getAddrBlock((char*)currentDirTable)) == -1)
        return -1;
    return 0;
}


//创建FCB
int creatFCB(int fcbBlockNum, int fileBlockNum, int fileSize)
{
    //找到fcb的存储位置
    FCB* currentFCB = (FCB*) getBlockAddr(fcbBlockNum);
    currentFCB->blockNum = fileBlockNum;//文件数据起始位置
    currentFCB->fileSize = fileSize;//文件大小
    currentFCB->link = 1;//文件链接数
    currentFCB->dataSize = 0;//文件已写入数据长度
    currentFCB->readptr = 0;//文件读指针


    currentFCB->count_sem = sem_open("count_sem", O_CREAT, 0644, NUMREADER);
    if(currentFCB->count_sem == SEM_FAILED)
    {
        perror("sem_open error");
        exit(1);
    }
    currentFCB->write_sem = sem_open("write_sem", O_CREAT, 0644, 1);
    if(currentFCB->write_sem == SEM_FAILED)
    {
        perror("sem_open error");
        exit(1);
    }
    return 0;
}


//添加目录项
int addDirUnit(dirTable* myDirTable, char fileName[], int type, int FCBBlockNum)
{
    //获得目录表
    int dirUnitAmount = myDirTable->dirUnitAmount;
    //检测目录表示是否已满
    if(dirUnitAmount == DIRTABLE_MAX_SIZE)
    {
        printf("dirTables is full, try to delete some file\n");
        return -1;
    }

    //是否存在同名文件
    if(findUnitInTable(myDirTable, fileName) != -1)
    {
        printf("file already exist\n");
           return -1;
    }
    //构建新目录项
    dirUnit* newDirUnit = &myDirTable->dirs[dirUnitAmount];
    myDirTable->dirUnitAmount++;//当前目录表的目录项数量+1
    //设置新目录项内容
    strcpy(newDirUnit->fileName, fileName);
    newDirUnit->type = type;
    newDirUnit->startBlock = FCBBlockNum;

    return 0;
}


//删除文件 rm
int deleteFile(char fileName[])
{
    //忽略系统的自动创建的父目录
    if(strcmp(fileName, "..") == 0)
    {
        printf("can't delete ..\n");
        return -1;
    }
    //查找文件的目录项位置
    int unitIndex = findUnitInTable(currentDirTable, fileName);
    if(unitIndex == -1)
    {
        printf("file not found\n");
        return -1;
    }
    dirUnit myUnit = currentDirTable->dirs[unitIndex];
    //判断类型
    if(myUnit.type == 0)//目录
    {
        printf("not a file\n");
        return -1;
    }
    int FCBBlock = myUnit.startBlock;
    //释放内存
    releaseFile(FCBBlock);
    //从目录表中剔除
    deleteDirUnit(currentDirTable, unitIndex);
    return 0;
}


//释放文件内存
int releaseFile(int FCBBlock)
{
    FCB* myFCB = (FCB*)getBlockAddr(FCBBlock);
    myFCB->link--;  //链接数减一
    //无链接，删除文件
    if(myFCB->link == 0)
    {
        //释放文件的数据空间
        releaseBlock(myFCB->blockNum, myFCB->fileSize);
    }
    //释放FCB的空间
    sem_close(myFCB->count_sem);
    sem_close(myFCB->write_sem);
    sem_unlink("count_sem");
    sem_unlink("write_sem");
    releaseBlock(FCBBlock, 1);
    return 0;
}


//删除目录项
int deleteDirUnit(dirTable* myDirTable, int unitIndex)
{
    //迁移覆盖
    int dirUnitAmount = myDirTable->dirUnitAmount;
    for(int i=unitIndex; i<dirUnitAmount-1; i++)
    {
        myDirTable->dirs[i] = myDirTable->dirs[i+1];
    }
    myDirTable->dirUnitAmount--;
    return 0;
}


//删除目录 rmdir
int deleteDir(char dirName[])
{
    //忽略系统的自动创建的父目录
    if(strcmp(dirName, "..") == 0)
    {
        printf("can't delete ..\n");
        return -1;
    }
    //查找文件
    int unitIndex = findUnitInTable(currentDirTable, dirName);
    if(unitIndex == -1)
    {
        printf("file not found\n");
        return -1;
    }
    dirUnit myUnit = currentDirTable->dirs[unitIndex];
    //判断类型
    if(myUnit.type == 0)//目录
    {
        deleteFileInTable(currentDirTable, unitIndex);
    }else {
        printf("not a dir\n");
        return -1;
    }
    //从目录表中剔除
    deleteDirUnit(currentDirTable, unitIndex);
    return 0;
}


//删除文件/目录项
int deleteFileInTable(dirTable* myDirTable, int unitIndex)
{
   //查找文件
    dirUnit myUnit = myDirTable->dirs[unitIndex];
    //判断类型
    if(myUnit.type == 0)//目录
    {
        //找到目录位置
        int FCBBlock = myUnit.startBlock;
        dirTable* table = (dirTable*)getBlockAddr(FCBBlock);
        //递归删除目录下的所有文件
        printf("cycle delete dir %s\n", myUnit.fileName);
        int unitCount = table->dirUnitAmount;
        for(int i=1; i<unitCount; i++)//忽略“..”
        {
            printf("delete %s\n", table->dirs[i].fileName);
            deleteFileInTable(table, i);
        }
        //释放目录表空间
        releaseBlock(FCBBlock, 1);
    }else {//文件
        //释放文件内存
        int FCBBlock = myUnit.startBlock;
        releaseFile(FCBBlock);
    }
    return 0;
}



//**********************读写操作*******************
FCB* my_open(char fileName[])
{
    int unitIndex = findUnitInTable(currentDirTable, fileName);
    if(unitIndex == -1)
    {
        printf("file no found\n");
        return NULL;
    }
    //控制块
    int FCBBlock = currentDirTable->dirs[unitIndex].startBlock;
    FCB* myFCB = (FCB*)getBlockAddr(FCBBlock);
    return myFCB;
}


//读文件
int my_read(char fileName[], int length)
{
    int unitIndex = findUnitInTable(currentDirTable, fileName);
    if(unitIndex == -1)
    {
        printf("file no found\n");
        return -1;
    }
    //控制块
    int FCBBlock = currentDirTable->dirs[unitIndex].startBlock;
    FCB* myFCB = (FCB*)getBlockAddr(FCBBlock);
    myFCB->readptr = 0; //文件指针重置
    //读数据
    char* data = (char*)getBlockAddr(myFCB->blockNum);
    int val;
    myFCB->count_sem = sem_open("count_sem", 0);
    /* 获取记录读者数量的锁 */
    if(sem_wait(myFCB->count_sem) == -1)
        perror("sem_wait error");
    sem_getvalue(myFCB->count_sem, &val);
    /* 根据拥有锁的进程数量来判断是否是第一个读者 */
    /* 如果是第一个读者就负责锁上写者锁 */
    if(val == NUMREADER-1)
    {
        myFCB->write_sem = sem_open("write_sem", 0);
        if(sem_wait(myFCB->write_sem) == -1)
            perror("sem_wait error");
    }

    int dataSize = myFCB->dataSize;
    /* printf("myFCB->dataSize = %d\n", myFCB->dataSize); */
    //在不超出数据长度下，读取指定长度的数据
    for(int i = 0; i < length && myFCB->readptr < dataSize; i++, myFCB->readptr++)
    {
        printf("%c", *(data+myFCB->readptr));
    }
    if(myFCB->readptr == dataSize)//读到文件末尾用#表示
        printf("#");
    /* 下面两行只是为了模拟编辑器的关闭之前的情况， */
    /* 这样就能控制进程不会立即释放锁 */
    printf("\ninput a character to end up reading....\n");
    getchar();
    /* 如果是最后一个读者就负责释放读者锁 */
    if(val == NUMREADER)
    {
        sem_post(myFCB->write_sem);
    }
    sem_post(myFCB->count_sem);
    printf("\n");
    return 0;
}


//写文件，从末尾写入 write
int my_write(char fileName[], char content[])
{
    int unitIndex = findUnitInTable(currentDirTable, fileName);
    if(unitIndex == -1)
    {
        printf("file no found\n");
        return -1;
    }
    //控制块
    int FCBBlock = currentDirTable->dirs[unitIndex].startBlock;
    FCB* myFCB = (FCB*)getBlockAddr(FCBBlock);
    /* myFCB->dataSize = 0; */
    /* myFCB->readptr = 0; */
    int contentLen = strlen(content);
    int fileSize = myFCB->fileSize * block_szie;
    char* data = (char*)getBlockAddr(myFCB->blockNum);
    myFCB->write_sem = sem_open("write_sem", 0);
    /* 获得写者锁 */
    if(sem_wait(myFCB->write_sem) == -1)
        perror("sem_wait error");
    //在不超出文件的大小的范围内写入
    for(int i = 0; i < contentLen && myFCB->dataSize < fileSize; i++, myFCB->dataSize++)
    {
        *(data+myFCB->dataSize) = content[i];
    }
    /* 模拟编辑器,控制写者不立即退出 */
    printf("input a character to end up waiting....\n");
    getchar();
    /* 释放写者锁 */
    sem_post(myFCB->write_sem);
    if(myFCB->dataSize == fileSize)
        printf("file is full, can't write in\n");
    return 0;
}



//从目录中查找目录项目
int findUnitInTable(dirTable* myDirTable, char unitName[])
{
    //获得目录表
    int dirUnitAmount = myDirTable->dirUnitAmount;
    int unitIndex = -1;
    for(int i = 0; i < dirUnitAmount; i++)//查找目录项位置
        if(strcmp(unitName, myDirTable->dirs[i].fileName) == 0)
            unitIndex = i;
    return unitIndex;
}


