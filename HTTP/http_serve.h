#ifndef HTTP_SERVE_H
#define HTTP_SERVE_H

#include <sys/epoll.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <mysql/mysql.h>
#include <map>
#include <stdio.h>
#include <errno.h>
#include "../Log/log.h"
#include "../User/user.h"
#include "../Mysql/connectpool.h"
#include "../Timer/timer.h"
#include "../Tool/tool.h"
#include "../Tool/locker.h"
#include "./parser.h"
#include "./state.h"

static const char *ok_200_title = "OK";
static const char *error_400_title = "Bad Request";        //Your request has bad syntax or is inherently impossible to staisfy
static const char *error_403_title = "Forbidden";          //You do not have permission to get file from this server
static const char *error_404_title = "Not Found";          //The requested file was not found on this server
static const char *error_500_title = "Internal Error";     //There was an unusual problem serving the request file

struct Data 
{
    string email;
    string password;
    Data() {/*需要先调用该默认构造函数*/}
    Data(const char* em, const char* pw) 
    :email(em), password(pw){}
};

struct file {
    string name;  //文件名
    int size;     //文件字节数
    char* addr;   //内存地址
    file() {/*需要先调用该默认构造函数*/}
    file(const char* name, int size , char* addr) 
    :name(name), size(size), addr(addr) {}
};


class http_serve {               
    int epoll;              //共享epoll
    int& clnt_num;          //共享clnt_num
    User* user;             //用户信息表
    Timer* timer;           //定时器
    Connectpool* sql_pool;  //数据库连接池
    const char* path;       //文件路径

private:    
    Parser parser;                 //主状态机(解析器)
    map<string, file> files;       //文件夹
    map<string, Data> data;        //用户信息的哈希表
    Mutexlock mutexlock;           //互斥锁

public:
    http_serve(int& clnt_num);
    ~http_serve();
    void process(int fd);           //总控函数
    void init(int epoll,const char* path, User* user, Timer* timer, Connectpool* sql_pool);
    void sql_get_data();
    int deal_request(int fd);      //最重要的业务处理函数-读取信息并服务
    bool write_in(int fd);          //向fd写入信息
    bool read_out(int fd);          //读出信息

private:
    void file_prepare();    //准备一些常用文件
    void add_file(const char* name, int limit = 0);     //向文件夹files中添加文件
    void mod_fd(int epoll, int fd, int events);         //在epoll中重新注册fd
    void iov_prepare(int fd, size_t size1, char* base1, size_t size2, char* base2);     //writev准备
    bool add_response(int fd, int status,               //添加响应头
                        const char* title, int content_length,   
                            const char* content_type, const char* charset, bool keep_alive);
};



#endif