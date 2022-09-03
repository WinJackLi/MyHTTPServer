#ifndef CONNECT_POOL
#define CONNECT_POOL

#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../Log/log.h"
#include "../Tool/locker.h"
#include "../Tool/tool.h"
#include "../config.h"

using namespace std;

class Connectpool 
{
public:
	bool release_conn(MYSQL *conn);          //释放连接
	int get_free_conn_num();				 //获取连接
	void destroy_pool();					 //销毁所有连接
	MYSQL *get_conn();				         //获取数据库连接
	
	static Connectpool *get_instance();												//单例模式，创建连接池
	void init(int port, string url, string user, string passwd, string database);   //初始化

private:
	Connectpool();
	~Connectpool();

private:
	int max_conn_num;       //最大连接数
	int cur_conn_num;       //当前已使用的连接数
	int free_conn_num;      //当前空闲的连接数
    Mutexlock mutexlock;    //使用互斥锁
	list<MYSQL*> pool;      //连接池
    Sem sem;				//信号量

private:
	string url;			 //主机地址
	string port;		 //数据库端口号
	string user;		 //登陆数据库用户名
	string passwd;	     //登陆数据库密码
	string database;     //使用数据库名
};

#endif