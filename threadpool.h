#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <iostream>
#include "locker.h"
#include "server.h"
using namespace std;
template<typename T>
class threadpool
{
    private:
        int m_thread_number;
        int m_max_request;
        pthread_t *m_threads;
        std::list<T*>m_workqueue;
        locker m_queuelocker;
        static void *worker(void *arg);
        sem m_queuestat;
        void run();
        int m_stop;
    public:
        void stop(){
            m_stop = 1;
        }
        int gettasksize(){
            return m_workqueue.size();
        }
        //threadpool()=default;
        threadpool(int thread_number=8,int max_requests=10000);
        ~threadpool();
        bool append(T* task);
};
template<typename T>
threadpool<T>::threadpool(int thread_number,int max_requests):
m_thread_number(thread_number),m_max_request(max_requests){
    if(thread_number <= 0 || max_requests <= 0){
        throw std::exception();
    }    
    m_stop = 0;
    m_threads = new pthread_t[m_thread_number];
    if(!m_threads){
        throw std::exception();
    }
    for(int i = 0;i < m_thread_number;++i){
        printf("the ith thread create\n");
        if(pthread_create(&m_threads[i],NULL,worker,this)!=0){
            delete []m_threads;
            throw std::exception();
        }
        if(pthread_detach(m_threads[i])){
            delete []m_threads;
            throw std::exception();
        }
	}
}
template<typename T>
threadpool<T>::~threadpool(){
    delete []m_threads;
    m_stop = true;
}
template<typename T>
bool threadpool<T>::append(T *request){
    m_queuelocker.lock();
    if(m_workqueue.size()>=m_max_request){
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuestat.post();
    m_queuelocker.unlock();
    return true;
}
template<typename T>
void* threadpool<T>::worker(void *arg){
    threadpool* pool = (threadpool*)arg;
    pool->run();
}
template<typename T>
void threadpool<T>::run()
{
    //cout<<"here"<<endl;
    while(!m_stop){
        m_queuestat.wait();
        m_queuelocker.lock();
        if(m_workqueue.empty()){
            m_queuelocker.unlock();
            continue;
        }        
        T* task = *m_workqueue.begin();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if(task==NULL)continue;
        task->run();
    }
}
#endif
