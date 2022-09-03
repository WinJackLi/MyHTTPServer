#ifndef TOOL_H
#define TOOL_H

#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <iostream>
#include <cassert>
#include <fcntl.h>
#include <sys/epoll.h>


//添加信号
void addsig(int sig);
   
//回调
void sig_handler( int sig );

//非阻塞fd,返回旧状态
int setnonblocking( int fd );

//输出错误
void perr(const char* the_error, const char* file_name = NULL, int line = 0);
    
//在epoll中删除fd    
void epoll_del(int epoll, int fd);
#endif