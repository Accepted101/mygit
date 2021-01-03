#include "threadpool.h"
#include <string>
using namespace std;
class cTask
{
    protected:
        string name;
        void *m_data;
    public:
        cTask(string &name):name(name),m_data(nullptr){}
        virtual void run() = 0;
        void print(){
            cout<<name<<endl;
        }
        void setdata(void *data){
            m_data = data;
        }
        virtual ~cTask(){}
};
class mytask:public cTask
{
    public:
        mytask(string name):cTask(name){}
        void run(){
            sleep(1);
            cout<<"ininin"<<endl;
            printf("my name is %s",(char*)m_data);
        }
};
int main()
{
    mytask task("1");
    char *p = "hello world";
    task.setdata(p);
    threadpool<mytask> mypoll;
    //sleep(10);
    for(int i = 1;i <= 10;++i){
        mypoll.append(&task);
    }
    while(1){
        printf("restask is %d\n",mypoll.gettasksize());
        if(mypoll.gettasksize()==0){
            mypoll.stop();
            break;
        }
        sleep(1);
    }
}