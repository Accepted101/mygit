#include <stdio.h>
#include <algorithm>
#include "server.h"
#include <pthread.h>
using namespace std;
struct conmsg
{
    int epollfd;
    int sockfd;
};
void setnonblocking(int);
void addfd(int epollfd,int fd,bool add)
{
    epoll_event events;
    events.data.fd = fd;
    events.events = EPOLLET|EPOLLIN;
    if(add)events.events|=EPOLLONESHOT;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&events);
    setnonblocking(fd);
}
void setnonblocking(int fd)
{
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
}
void reset_oneshot(int epollfd,int fd)
{
    epoll_event events;
    events.data.fd = fd;
    events.events = EPOLLET|EPOLLONESHOT|EPOLLIN;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&events);
}

void* worker(void *arg)
{
    conmsg* msg = (conmsg*)arg;
    int sockfd = msg->sockfd;
    int epollfd = msg->epollfd;
    printf("new thread to solve the%d\n",sockfd);
    char buf[1024];
    memset(buf,'\0',sizeof(buf));
    while(1){
        int ret = recv(sockfd,buf,1023,0);
        if(ret==0){
            close(sockfd);
            printf("connection closed\n");
        }
        else if(ret<0){
            if(errno==EAGAIN){
                reset_oneshot(epollfd,sockfd);
                printf("read later\n");
                break;
            }
        }
        else {
            printf("recive the %s\n",buf);
            sleep(5);
        }
    }
    printf("end recive data on fd %d\n",sockfd);
}
int main(int argc,char* argv[])
{
    const int port = atoi(argv[2]);
    const char *ipaddr = argv[1];
    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ipaddr);
    server.sin_port = htons(port);
    int sock = socket(AF_INET,SOCK_STREAM,0);
    bind(sock,(struct sockaddr*)&server,sizeof(server));
    listen(sock,10);
    int epollfd = epoll_create(100);
    epoll_event events[1024];
    addfd(epollfd,sock,false);
    while(1){
        int cnt = epoll_wait(epollfd,events,1024,-1);
        for(int i = 0;i < cnt;++i){
            if(events[i].data.fd==sock){
                struct sockaddr_in cilent;
                socklen_t len;
                int confd = accept(sock,(struct sockaddr*)&cilent,&len);
                addfd(epollfd,confd,true);
                printf("ok connect\n");
            }
            else if(events[i].events&EPOLLIN) {
                pthread_t thread;
                conmsg tmp;
                tmp.epollfd = epollfd,tmp.sockfd = events[i].data.fd;
                pthread_create(&thread,NULL,worker,(void*)&tmp);
            }
        } 
    }
    close(sock);
    return 0;
}