#ifndef CONFIG_H
#define CONFIG_H

//服务器配置
#define MAX_CLNTS 65536
#define EPOLL_SIZE 10000

//异步日志系统配置
#define MAX_LOG_FILE 104857600   //单个日志文件最大为100mb
#define MAX_TASK_NUM 2000        //任务阻塞队列的长度

//用户信息配置
#define READABLE 0
#define WRITABLE 1


//定时器配置
#define TIMEOUT 15
#define SHORT_TICK 5


//线程池配置
#define POOL_SIZE 8              //线程池数量
#define MAX_POOL_SIZE 10         
#define ALIVE 0
#define DEAD 1
#define WORKING 2
#define TIME_TO_SLEEP 10         //定义了管理者线程的检查时间间隙，检查并更新线程池容量
#define TIME_TO_ADD 2            //当 任务数 > 工作线程数 * 2 时，用新线程替换死亡的线程
#define TIME_TO_EXPAND 4         //当 任务数 > 工作线程数 * 4 时，扩展线程池至最大线程
#define TIME_TO_DEL 2            //当 任务数 < 工作线程数 / 2 时，杀死部分线程，至POOL_SIZE

//线程池错误代码
#define PTHREAD_CREATE_FAILED 1  //create失败
#define PTHREAD_DETCH_FAILED 2   //detach 失败
#define PTHREAD_CANCEL_FAILED 1  //cancel失败
#define PTHREAD_DELETE_FAILED 2  //删除失败，没有alive线程


//缓冲区
#define READ_BUF_SIZE 1024        //客户连接的读数据缓冲区大小
#define WRITE_BUF_SIZE 512        //客户连接的写数据缓冲区大小
#define CONTENT_SIZE 128          //post表单大小
#define ROOT_SIZE 64              //文件根的缓冲大小
#define PATH_SIZE 128             //客户请求的文件路径缓冲大小
#define FILE_NAME_SIZE 64         //文件名缓冲大小


//读写锁配置
#define READER 0  //读优先(默认)
#define WRITER 1  //写优先(默认)

//数据库连接池配置
#define MAX_CONN_NUM 8
#endif