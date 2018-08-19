#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "File.h"
#include "Disk.h"

/* 获得当前目录名 */
void GetDir()
{
    char *path = getPath();
    printf("%s", path);
}

/* ctrl-C的信号处理程序 */
static void sigHandler(int sig)
{
    exitSystem();
    exit(0);
}


int main()
{
    initSystem();
    if(signal(SIGINT, sigHandler) == SIG_ERR)
    {
        perror("signal");
        exit(1);
    }

    initRootDir();

    while(1)
    {
        GetDir();
        fflush(stdout);
        //读取字符串
        char buf[1024];
        int s = read(0, buf, 1024);
        if(s > 0)//有读取到字符
        {
            int i = 0;
            for( i = 0; i < s; ++i)
            {
                if(buf[i] == '\b' && i >= 1)
                {
                //    printf("debug:%d\n",i);
                    int j = 0;
                    for( j = i+1; j < s; ++j)
                    {
                        buf[j-2] = buf[j];
                    }
                    buf[s-2] = 0;
                    i-=1;
                }
                else if(buf[i] == '\b' && i == 0)
                {
                //    printf("debug:%d\n",i);
                    int j = 0;
                    for( j = 1; j < s; ++j)
                    {
                        buf[j-1] = buf[j];
                    }
                    buf[s-1] = 0;
                //    i-=1;
                }
                else
                {
                    continue;
                }
            }
            buf[s] = 0;
        }
        else
        {
            continue;
        }
        //将读取到的字符串分成多个字符串
        char* start = buf;
        int i =1;
        char* MyArgv[10] = {0};
        MyArgv[0] = start;
        /* 跳过空格 */
        while(*start)
        {
            if(isspace(*start))
            {
                *start = 0;
                start++;
                MyArgv[i++] = start;
            }
            else
            {
                ++start;
            }
        }
        MyArgv[i-1] = NULL;

        if(!strcmp(MyArgv[0], "ls"))
        {
            showDir();
        }
        else if(!strcmp(MyArgv[0], "cd"))
        {
            if(MyArgv[1] != NULL)
                changeDir(MyArgv[1]);
            else
            {
                printf("usage:cd dirname\n");
            }
        }
        else if(!strcmp(MyArgv[0], "mv"))
        {
            if(MyArgv[1] != NULL && MyArgv[2] != NULL)
                changeName(MyArgv[1], MyArgv[2]);
            else
            {
                printf("usage:mv oldname, newname\n");
            }
        }
        else if(!strcmp(MyArgv[0], "touch"))
        {
            if(MyArgv[1] != NULL && MyArgv[2] != NULL)
                creatFile(MyArgv[1], atoi(MyArgv[2]));
            else
            {
                printf("usage:touch filename filesize\n");
            }
        }
        else if(!strcmp(MyArgv[0], "mkdir"))
        {
            if(MyArgv[1] != NULL)
                creatDir(MyArgv[1]);
            else
            {
                printf("usage:mkdir dirname\n");
            }
        }
        else if(!strcmp(MyArgv[0], "rm"))
        {
            if(MyArgv[1] != NULL)
                deleteFile(MyArgv[1]);
            else
            {
                printf("usage:rm filename\n");
            }
        }
        else if(!strcmp(MyArgv[0], "rmdir"))
        {
            if(MyArgv[1] != NULL)
                deleteDir(MyArgv[1]);
            else
            {
                printf("usage:rmdir filename\n");
            }
        }
        else if(!strcmp(MyArgv[0], "my_read"))
        {
            if(MyArgv[1] != NULL && MyArgv[2] != NULL)
                my_read(MyArgv[1], atoi(MyArgv[2]));
            else
            {
                printf("usage:my_read filename size\n");
            }
        }
        else if(!strcmp(MyArgv[0], "my_write"))
        {
            if(MyArgv[1] != NULL && MyArgv[2] != NULL)
                my_write(MyArgv[1], MyArgv[2]);
            else
            {
                printf("usage:my_write filename content\n");
            }
        }
        else if(!strcmp(MyArgv[0], "exit"))
        {
            exitSystem();
            exit(0);
        }
        else
        {
            printf("error:command not found\n");
        }
    }
    return 0;
}
