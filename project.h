#ifndef __PROJECT_H__
#define __PROJECT_H__
#include<stdio.h>
#include<error.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sqlite3.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/time.h>

#define MSGPATH "/home/linux/HQYJ/"

#define ERR_LOG(msg) do{\
    printf("%s :%s :%d\n",__FILE__,__func__,__LINE__);  \
    perror(msg);  \
    return -1;  \
}while(0);  

sem_t sem;

typedef struct _Node{
    pthread_t tid;
    int idsum;
    int fd;       
    struct _Node *next;
}node_t;

//消息队列结构体
typedef struct{
    char ID[20];
    char PS[10];
    char flags;
}login_t;

typedef struct{
	float temp;
	unsigned char hume;
	unsigned short lux;
	unsigned char devstart;
} envdata_t;

typedef struct{
	float temp_up;
	float temp_down;
	unsigned char hume_up;
	unsigned char hume_down;
	unsigned short lux_up;
	unsigned short lux_down;

} limitset_t;

typedef struct{
	long msgtype;
	long recvmsgtype;
	/*登录信息*/
	login_t log;
	
	/*环境数据*/
	envdata_t env;

	/*阈值数据*/
	limitset_t setdata;
	/*控制数据*/
	unsigned char devctl;

	/*指令数据*/
	unsigned char commd;
} ipc_msg_t;

#define LOGIN 1
#define CLOSE 0

void *task_ipcs_func(void *arg);
int delete_pfd(node_t *pfdtemp);
void *inspect_ID_func(void *argv);
int delete_fd(node_t *pfd, int fd);
void *task_func(void *arg);
int Net_init(const char *PROT);
#endif