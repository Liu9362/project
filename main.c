#include "project.h"

pthread_t tid;
int msqidA;
int msqidB;
int clientfd;
struct sockaddr_in serveraddr,clientaddr;
node_t *pfdhead;
// 需要连接线程库  -lpthread
int main(int argc, char const *argv[])
{
    int ret = 0;
    if(argc < 2){
		puts("请输入服务器端口号");
		return -1;
	}

    // tcp流程
	int sockfd = Net_init(argv[1]);
	if(-1==sockfd){
		return -1;
	}

    pfdhead = (node_t *)malloc(sizeof(node_t));
    if (NULL == pfdhead)
    {
        printf("内存分配失败\n");
        close(sockfd);
        return -1;
    }
    pfdhead->idsum = 0;
    pfdhead->next = pfdhead;

    // 信号量
    sem_init(&sem, 0, 1);
    int acceptfd = 0;

    //创建消息队列
    ipc_msg_t ipcmsg;
    long type=0;
    //读消息队列
    key_t my_keyA;
    my_keyA = ftok(MSGPATH, 'A');
    msqidA = msgget(my_keyA, IPC_CREAT|0666);
    //发送消息队列
    key_t my_keyB;
    my_keyB = ftok(MSGPATH, 'B');
    msqidB = msgget(my_keyB, IPC_CREAT|0666);

    //心跳包线程
	if(0!=(ret=pthread_create(&tid,NULL,inspect_ID_func,NULL)))
    {
            printf("pthread_create inspect_ID_func error : %s\n", strerror(ret));
            exit(-1);
        }
	pthread_detach(tid);
    printf("心跳包线程创建成功\n");
    int clientaddr_len = sizeof(clientaddr);
    while (1)
    {
        sem_wait(&sem);
        // 阻塞等待客户端连接
        printf("阻塞等待客户端连接\n");
        if (-1 == (acceptfd = accept(sockfd, (struct sockaddr *)&clientaddr, &clientaddr_len)))
            ERR_LOG("accept error");
        printf("客户端连接\n");
        //fd_node_t* pnewfd=NULL;
        //pnewfd=(fd_node_t*)malloc(sizeof(fd_node_t));
        //pnewfd->fd=acceptfd;
        //pnewfd->next=pheadfd->next;
        //pheadfd->next=pnewfd;
        if (0 != (ret = pthread_create(&tid, NULL, task_func,&acceptfd)))
        {
            printf("pthread_create task_funcerror : %s\n", strerror(ret));
            exit(-1);
        }
        pthread_detach(tid);
    }
    // 销毁无名信号量
    sem_destroy(&sem);
    close(sockfd);

    return 0;
}
