#ifndef MYTIME_H
#define MYTIME_H
#include <time.h>
#include "server.h"
#define buffer_size 64
class util_timer;
struct user_data
{
    /* data */
    sockaddr_in addr;
    int sockfd;
    char buf[buffer_size];
    util_timer* timer;
};

class util_timer
{
    public:
    public:
        time_t expire;
        void (*cb_func)(user_data*);
        user_data *data;
        util_timer *prev;
        util_timer *next;
};

class sort_timer_list
{
    public:
        sort_timer_list():head(NULL),tail(NULL){};
        ~sort_timer_list()
        {
            util_timer *tmp = head;
            while(tmp!=NULL){
                head = tmp->next;
                delete tmp;
                tmp = head;
            }
        }
        void add_timer(util_timer *timer){
            if(!timer)return ;
            if(!head)head = tail = timer;
            if(timer->expire < head->expire){
                timer->next = head;
                head->prev = timer;
                head = timer;
                head->prev = NULL;
                return;
            }
            add_timer(timer,head);
        }
        void adjust_timer(util_timer* timer){
            if(!timer)return;
            util_timer* tmp = timer->next;
            if(!tmp||timer->expire<tmp->expire)return;
            if(timer==head){
                head = head->next;
                head->prev = NULL;
                timer->next = NULL;
                add_timer(timer,head);
                return;
            }
            else{
                timer->prev->next = timer->next;
                timer->next->prev = timer->prev;
                add_timer(timer,timer->next);
            }
        }
        void deltimer(util_timer* timer){
            if(!timer)return;
            if((timer==head)&&(timer==tail)){
                delete timer;
                head = NULL;
                tail = NULL;
                return;
            }
            if(timer==head){
                head = head->next;
                head->prev = NULL;
                delete timer;
                return;
            }
            if(timer==tail){
                tail = tail->prev;
                tail->next = NULL;
                delete timer;
                return;
            }
            timer->next->prev = timer->prev;
            timer->prev->next = timer->next;
            delete timer;
        }
        void tick()
        {
            if(!head)return;
            printf("timer tick!\n");
            time_t now = time(NULL);
            util_timer* tmp = head;
            while(tmp){
                if(now < tmp->expire){
                    break;
                }
                tmp->cb_func(tmp->data);
                head = tmp->next;
                if(head)head->prev=NULL;
                delete tmp;
                tmp = head;
            }

        }
    private:
        void add_timer(util_timer* timer,util_timer* head){
            util_timer* pre,*tmp;
            pre = head;
            tmp = head->next;
            while(tmp){
                if(timer->expire < tmp->expire){
                    pre->next = timer;
                    timer->prev = pre;
                    timer->next = tmp;
                    tmp->prev = timer;
                    break;
                }
                pre = tmp;
                tmp = pre->next;
            }
            if(!tmp){
                pre->next = timer;
                timer->next = NULL;
                timer->prev = pre; 
                tail = timer;
            }
        }
        util_timer *head,*tail;
};
#endif