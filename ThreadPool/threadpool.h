#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <queue>
#include <vector>
#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include "../Mysql/connectpool.h"
#include "../HTTP/http_serve.h"
#include "../Log/log.h"
#include "../Timer/timer.h"
#include "../Tool/locker.h"
#include "../Tool/tool.h"
#include "../User/user.h"
#include "../config.h"

struct Thread {
    pthread_t id;
    int state;
    Thread() {
        id = 0;
        state = ALIVE;
    }
};



class Threadpool
{
public:
	static Threadpool* get_instance();
	void destroy_pool();					 //销毁所有连接
    void init(int epoll, User* user, Timer* timer, Connectpool* sql_pool, http_serve* serve);
    void add_task(int fd);  //添加任务

private:
    Threadpool();
    ~Threadpool();
    void worker();                              //工作函数
    static void* start( void* arg );            //工作函数启动函数
    /*
    void manager_worker();                      //管理者线程的工作函数
    static void* manager_start( void* arg);     //管理者线程
    */
    int add_thread(); //增加一条活跃的线程，线程不安全，成功:0，失败:错误码
    int del_thread(); //销毁一条活跃的线程，线程不安全，成功:0，失败:错误码
    int get_task();

private:
    Mutexlock mutexlock;
    Spinlock spinlock;
    Sem sem;

private:
    User* user;             //用户信息表 
    Timer* timer;           //主要的业务处理类，调用服务器创建的定时器
    Connectpool* sql_pool;  //连接池
    http_serve* serve;      //http服务 
    int epoll;              //服务器epoll
    int pool_size;          //限制总线程数
    int max_pool_size;      //最大总线程数
    int alive_thread;       //存活等待任务的线程数
    int dead_thread;        //已死亡的线程数
    int working_thread;     //工作中的线程数
    int task_num;           //任务数
    bool close_pool;             //关闭线程池
    
    Thread manager_thread;  //管理线程
    vector<Thread> pool;    //线程池
    queue<int> task_queue;  //任务队列
};

#endif