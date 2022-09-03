#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <cassert>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <list>
#include "../Log/log.h"
#include "../config.h"
#include "../Timer/timer.h"
#include "../HTTP/http_serve.h"
#include "../ThreadPool/threadpool.h"
#include "../Mysql/connectpool.h"
#include "../User/user.h"
#include "../Tool/tool.h"
#include "../Tool/locker.h"

extern int pipe_fd;

class Server {
	int port;
	int serv_sock;
	int epoll;
	int clnt_num;					//用户数量
	bool stop_server;				//是否关闭服务器
	Timer* timer;				 	//在栈区创建定时器服务
	bool timeout;					//是否需要处理定时事件
	int pipe[2];					//与定时器交流的管道
	User *user;					    //在堆区创建用户信息表
	http_serve* serve;				//http服务程序
	Threadpool*  pool;				//单例模式创建线程池
	Connectpool* sql_pool;			//单例模式创建数据库连接池
	struct epoll_event* events;		//epoll事件集合

public:		
	Server();
	~Server();
	void event_loop();
	void init(int port, const char* path, int sql_port, string sql_url, string sql_user, string sql_database, string sql_passwd);

private:
	void sock_init();             //初始化服务器套接字
	void epoll_init();			  //初始化epoll
	void epoll_add( int fd, bool one_shot, int mode);
	bool deal_signal(bool &timeout, bool &stop_server);
	int accept_conn_request(); 	  //接受用户连接
};


#endif
