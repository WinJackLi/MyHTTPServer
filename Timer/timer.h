#ifndef TIMER_H
#define TIMER_H

#include <time.h>
#include <iostream>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <vector>
#include <string.h>
#include <cassert>
#include "../Log/log.h"
#include "../Tool/tool.h"
#include "../config.h"

void cb_func(int epoll, int fd);

struct client {
    int fd;
    int id;
    time_t timeout;
    void (*cb_func)(int epoll, int fd);
    client():fd(-1), id(0), timeout(0), cb_func(NULL) {}
};


class Timer {
private:
    int rear;                           //堆底元素索引，也是定时器的数量
    int epoll;                          //用于修改epoll
    int& clnt_num;
    client* heap[MAX_CLNTS];
    client clients[MAX_CLNTS];         //维护客户端信息，便于索引修改timeout
    
    bool heap_add(client *clnt);        //添加定时器
    bool heap_del();                    //删除堆顶定时器
    bool clean_timer(client *clnt);     //延迟单独删除定时器
    bool heap_del(int id);              //单独删除定时器
    void heap_adjust(int i);            //整理时间堆

public:
    Timer(int& clnt_num, int epoll);
    ~Timer();
    bool sign_in(int fd, int timeout = TIMEOUT); //注册定时器
    bool log_out(int fd);                        //注销定时器
    bool update(int fd, int timeout = TIMEOUT);  //更新定时器的时间
    void start();                                //开启定时器服务
    void tick();                                 //检查并删除超时定时器
};


#endif