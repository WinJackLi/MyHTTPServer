#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <queue>
#include <iostream>
#include <string>
#include <cassert>
#include <pthread.h>
#include <time.h>
#include "../Tool/locker.h"
#include "../config.h"
#define LOG(type,format,...) {Log::get_instance()->log(type, format,##__VA_ARGS__);}

struct LogTask {
    std::string type;
    std::string content;
};

class Log
{
    int number;
    FILE* fp;                   //日志文件
    struct tm cur;              //当前时间  
    struct stat file_stat;      //文件大小
    std::string path;           //文件路径
    std::string second;
    int task_num;               //阻塞的任务数
    Mutexlock mutex;            //互斥锁
    std::queue<LogTask> task_queue;  //待写入的任务队列
    Sem sem;

public:
    static Log *get_instance();
    void start();
    bool log(string type, const char * format, ...);

private:
    Log();
    ~Log();
    bool get_new_fp();
    void worker();
    static void* strike(void * args);
};

#endif