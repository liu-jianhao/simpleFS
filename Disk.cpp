#include "Disk.h"

#include<stdio.h>
#include<stdlib.h>



char* systemStartAddr;  //系统起始地址

//初始化系统
void initSystem()
{
    /* 分配100M空间 */
    systemStartAddr = (char*)malloc(system_size * sizeof(char));
    //初始化盘块的位示图
    for(int i = 0; i < block_count; i++)
        systemStartAddr[i] = '0';
    //用于存放位示图的空间已被占用
    int bitMapSize = block_count * sizeof(char) / block_szie;//位示图占用盘块数:100
    for(int i=0; i<bitMapSize; i++)//从零开始分配
        systemStartAddr[i] = '1';   //盘块已被使用
}

//退出系统
void exitSystem()
{
    free(systemStartAddr);
}

//磁盘分配
int getBlock(int blockSize)
{
    int startBlock = 0;
    int sum = 0;
    for(int i = 0; i < block_count; i++)
    {
        if(systemStartAddr[i] == '0')//可用盘块
        {
            if(sum == 0)//刚开始，设置开始盘块号
                startBlock = i;
            sum++;
            if(sum == blockSize)//连续盘块是否满足需求
            {
                //满足分配，置1
                for(int j = startBlock; j < startBlock+blockSize; j++)
                    systemStartAddr[j] = '1';
                return startBlock;
            }

        }
        else//已被使用,连续已经被打断
            sum = 0;
    }
    printf("not found such series memory Or memory is full\n");
    return -1;
}
//获得盘块的物理地址
char* getBlockAddr(int blockNum)
{
    return systemStartAddr + blockNum * block_szie; //偏移量单位为字节
}
//获得物理地址的盘块号
int getAddrBlock(char* addr)
{
    return (addr - systemStartAddr)/block_szie;
}
//释放盘块、
int releaseBlock(int blockNum, int blockSize)
{
    int endBlock = blockNum + blockSize;
    //修改位示图盘块的位置为0
    for(int i = blockNum; i < endBlock; i++)
        systemStartAddr[i] = '0';
    return 0;
}
