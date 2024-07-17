#include "project.h"

int flag = 0;
ipc_msg_t ipcmsg;
extern node_t *pfdhead;
extern int msqidA;
extern int msqidB;
int clientfd;
extern struct sockaddr_in serveraddr, clientaddr;
struct timeval sigtm;
struct timeval timeout;
int Net_init(const char *PROT)
{

    int sockfd = (sockfd = socket(AF_INET, SOCK_STREAM, 0));
    if (sockfd < 0)
        ERR_LOG("socket error");

    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // inet_addr(argv[1]);
    serveraddr.sin_port = htons(atoi(PROT));
    serveraddr.sin_family = AF_INET;
    socklen_t serveraddr_len = sizeof(serveraddr);

    // 允许端口复用

    if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    if (-1 == (bind(sockfd, (struct sockaddr *)&serveraddr, serveraddr_len)))
    {
        ERR_LOG("bind error");
        close(sockfd);
    }
    if (-1 == (listen(sockfd, 10)))
    {
        ERR_LOG("listen error");
        close(sockfd);
    }
    return sockfd;
}

// 处理线程
void *task_func(void *arg)
{

    printf("chuli\n");
    int clientfd = *((int *)arg);
    // pthread_detach(pthread_self());
    // sem_post(&sem);
    ipc_msg_t client_msg;
    memset(&client_msg, 0, sizeof(ipc_msg_t));
    int nbytes = 0;
    // 下位机
    if (-1 == (nbytes = recv(clientfd, &client_msg, sizeof(client_msg), 0)))
    {
        perror("recv error");
        close(clientfd);
        pthread_exit(NULL);
    }
    else if (nbytes == 0)
    {
        printf("下位机断开连接了..\n");
        close(clientfd);
        pthread_exit(NULL);
    }

    printf("1recv[%s]\n", client_msg.log.ID);
    int i = 0;
    long idsum = 0;
    while (client_msg.log.ID[i])
    {
        idsum += client_msg.log.ID[i];
        i++;
    }
    node_t *pnew = (node_t *)malloc(sizeof(node_t));
    if (NULL == pnew)
    {
        printf("内存分配失败\n");
        close(clientfd);
        pthread_exit(NULL);
    }
    pfdhead->fd = clientfd;
    pfdhead->tid = pthread_self();
    pnew->next = pfdhead->next;
    pfdhead->next = pnew;

    memset(&client_msg, 0, sizeof(ipc_msg_t));
    msgrcv(msqidA, &client_msg, sizeof(client_msg) - sizeof(long), 1, 0);
    printf("读取到消息%s\n", client_msg.log.ID);
    if (client_msg.recvmsgtype == idsum)
    {
        printf("%d:登录成功\n", clientfd);
        ipcmsg.msgtype = idsum;
        client_msg = ipcmsg;
        client_msg.log.flags = 1;
        send(clientfd, &client_msg, sizeof(ipc_msg_t), 0);
        msgsnd(msqidB, &client_msg, sizeof(client_msg) - sizeof(long), 0);
    }
    else
    {
        msgsnd(msqidA, &ipcmsg, sizeof(ipcmsg) - sizeof(long), 0);
    }

    /*数据处理*/
    while (1)
    {
        memset(&ipcmsg, 0, sizeof(ipc_msg_t));
        msgrcv(msqidA, &ipcmsg, sizeof(ipc_msg_t) - sizeof(long), idsum, 0);
        printf("%ld消息到来，指令%d\n", idsum, ipcmsg.commd);
        send(clientfd, &ipcmsg, sizeof(ipc_msg_t), 0);

        /*等待智能硬件返回数据*/
        nbytes = recv(clientfd, &ipcmsg, sizeof(ipc_msg_t), 0);
        if (-1 == nbytes)
        {
            perror("recv error");
            delete_pfd(pnew);
        }
        else if (nbytes == 0)
        {
            printf("下位机断开连接了.淫秽.\n");
            delete_pfd(pnew);
        }

        /*将数据发送到 B队列*/
        ipcmsg.msgtype = idsum;
        msgsnd(msqidB, &ipcmsg, sizeof(ipc_msg_t) - sizeof(long), 0);
    }
}

int delete_fd(node_t *pfd, int fd)
{
    node_t *ptemp = pfd;
    node_t *pdel = NULL;
    while (ptemp->next)
    {
        if (ptemp->next->fd == fd)
        {
            pdel = ptemp->next;
            ptemp->next = pdel->next;
            free(pdel);
            return 0;
        }
        ptemp = ptemp->next;
    }
    return -1;
}

// 心跳包线程
void *inspect_ID_func(void *argv)
{
    node_t *pfdtemp = pfdhead;
    ipc_msg_t buf;
    int nbytes = 0;
    // 设置接收超时时间
    char survival_buff[4] = {0};
    while (1)
    {

        if (pfdtemp->next->fd != 0)
        {

            puts("20S后检查下位机是否在线");
            sleep(20); // 每20s检查客户端是否存活
            sigtm.tv_sec = 3;
            sigtm.tv_usec = 0;

            setsockopt(pfdtemp->next->fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&sigtm, sizeof(sigtm));
            buf.commd = 255;
            buf.log.flags = 1;
            if (-1 == send(pfdtemp->next->fd, &buf, sizeof(ipc_msg_t), 0))
            {
                perror("send error");
            }
            printf("心跳包发送\n");
            if (0 >= (nbytes = recv(pfdtemp->next->fd, &buf, sizeof(ipc_msg_t), 0)))
            {
                sigtm.tv_sec = 0;
                sigtm.tv_usec = 0;
                setsockopt(pfdtemp->next->fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&sigtm, sizeof(sigtm));
                printf("客户端%d超时\n", pfdtemp->next->fd);
                delete_pfd(pfdtemp);
            }
            else
            {
                sigtm.tv_sec = 0;
                sigtm.tv_usec = 0;
                setsockopt(pfdtemp->next->fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&sigtm, sizeof(sigtm));
                pfdtemp = pfdtemp->next;
            }
        }
        else
        {
            pfdtemp = pfdtemp->next;
        }
    }
}

int delete_pfd(node_t *pfdtemp)
{
    pthread_cancel(pfdtemp->next->tid);
    close(pfdtemp->next->fd);
    node_t *pdel = pfdtemp->next;
    pfdtemp->next = pdel->next;
    free(pdel);
    pdel = NULL;
    return 0;
}