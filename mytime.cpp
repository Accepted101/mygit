#include "mytime.h"
#include <algorithm>
#include <signal.h>
#include <unistd.h>
using namespace std;
#define timeslot 5
static int pipefd[2];
static sort_timer_list time_list;
static int epollfd = 0;
int setnonblocking(int fd)
{
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}
void addfd(int epollfd,int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN|EPOLLET;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}
void sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1],(char*)&msg,1,0);
    errno = save_errno;
}
void add_sig(int sig)
{
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    sigaction(sig,&sa,NULL);
}
void timer_handler()
{
    time_list.tick();
    alarm(timeslot);
}
void cb_func(user_data* user)
{
    epoll_ctl(epollfd,EPOLL_CTL_DEL,user->sockfd,0);
    close(user->sockfd);
    printf("close the %d\n",user->sockfd);
}
int main(int argc,char* argv[])
{
    const char *ip = argv[1];
    const int port = atoi(argv[2]);
    struct sockaddr_in address;
    memset(&address,0,sizeof(address));
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    int listenfd = socket(AF_INET,SOCK_STREAM,0);
    bind(listenfd,(struct sockaddr*)&address,sizeof(address));
    listen(listenfd,10);
    epoll_event events[1025];
    int epollfd = epoll_create(10);
    addfd(epollfd,listenfd);
    socketpair(PF_UNIX,SOCK_STREAM,0,pipefd);
    setnonblocking(pipefd[1]);
    addfd(epollfd,pipefd[0]);
    add_sig(SIGALRM);
    add_sig(SIGTERM);
    bool stop = false;
    user_data* users = new user_data[1024];
    bool timeout = false;
    alarm(timeslot);
    while(!stop){
        int cnt = epoll_wait(epollfd,events,1024,-1);
        if((cnt < 0)&&(errno!=EINTR)){
            printf("epoll error!\n");
            break;
        }
        for(int i = 0;i < cnt;++i){
            int sockfd = events[i].data.fd;
            if(sockfd==listenfd){
                struct sockaddr_in cilent;
                socklen_t cilent_len = sizeof(cilent);
                int confd = accept(listenfd,(struct sockaddr*)&cilent,&cilent_len);
                addfd(epollfd,confd);
                users[confd].sockfd = confd;
                users[confd].addr = cilent;
                util_timer* timer = new util_timer;
                timer->data = &users[confd];
                timer->cb_func = cb_func;
                time_t cur = time(NULL);
                timer->expire = cur+3*timeslot;
                users[confd].timer = timer;
                time_list.add_timer(timer);
            }
            else if(sockfd==pipefd[0]&&(events[i].events&EPOLLIN)){
                int sig;
                char signals[1024];
                int res = recv(pipefd[0],signals,sizeof(signals),0);
                if(res==-1){
                    printf("error!\n");
                    break;
                }
                else if(res==0){
                    continue;
                }
                else{
                    for(int i = 0;i < res;++i){
                        if(signals[i]==SIGTERM){
                            stop = true;
                        }
                        else if(signals[i]==SIGALRM){
                            timeout = true;
                        }
                    }
                }
            }
            else if(events[i].events&EPOLLIN){
                memset(users[i].buf,'\0',sizeof(users[i].buf));
                int ret = recv(sockfd,users[i].buf,sizeof(users[i].buf)-1,0);
                printf("%d %s %d\n",ret,users[i].buf,sockfd);
                util_timer* timer = users[i].timer;
                if(ret < 0){
                    if(errno!=EAGAIN){
                        cb_func(&users[sockfd]);
                        if(timer){
                            time_list.deltimer(timer);
                        }
                    }
                }
                else if(ret==0){
                    cb_func(&users[sockfd]);
                    if(timer){
                        time_list.deltimer(timer);
                    }
                }
                else{
                    if(timer){
                        time_t cur = time(NULL);
                        timer->expire = cur+3*timeslot;
                        printf("adjust time once\n");
                        time_list.adjust_timer(timer);

                    }
                }
            }
        }
        if(timeout){
            timer_handler();
            timeout = false;
        }
    }
}