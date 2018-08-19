#ifndef DISKOPERATE_H_INCLUDED
#define DISKOPERATE_H_INCLUDED

#define system_size 100*1024*1024           //系统大小
#define block_szie 1024                     //盘块大小
#define block_count system_size/block_szie  //系统盘块数目

/* 初始化系统 */
void initSystem();
/* 磁盘分配 */
int getBlock(int blockSize) ;
/* 获得盘块的物理地址 */
char* getBlockAddr(int blockNum);
/* 获得物理地址的盘块号 */
int getAddrBlock(char* addr);
/* 释放盘块、 */
int releaseBlock(int blockNum, int blockSize);
/* 退出系统 */
void exitSystem();

#endif
