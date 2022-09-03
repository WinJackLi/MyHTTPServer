#include "./log.h"

Log* Log::get_instance() 
{
    //单例模式
    static Log mylog;
    return &mylog;
}

Log::Log() 
{
    number = 1;
    get_new_fp();
}

    
Log::~Log() 
{
    if (fp) fclose(fp);
}
    

void Log::start() 
{
    pthread_t id;
    pthread_create(&id, NULL, strike, NULL);
    pthread_detach(id);
}


bool Log::get_new_fp()
{
    //打开日志文件，先检测该文件是否超过100mb
    char root[ROOT_SIZE] = {0};
    char file_time[20] = {0};
    getcwd(root, ROOT_SIZE);
    const char* tar = "/Log/logs/";
    strcat(root, tar);
    //time
    time_t t = time(NULL);
    cur = *localtime(&t);
    snprintf(file_time, 20, "%d.%02d.%02d", cur.tm_year+1900, cur.tm_mon + 1, cur.tm_mday);
    while (true)
    {
        char temp[PATH_SIZE] = {0};
        char file_name[20] = {0};
        struct stat file_stat;
        snprintf(file_name, 20, "%s%d%s", "_LogFile", number, ".log");
        snprintf(temp, PATH_SIZE, "%s%s%s", root, file_time, file_name);
        if(stat(temp, &file_stat) < 0) {
            //文件不存在
            fp = fopen(temp, "a");
            assert(fp != NULL);
            this->path = temp;
            break;
        }
        else {
            if(file_stat.st_size > MAX_LOG_FILE) 
            {
                number++;
            }
            else {
                fp = fopen(temp, "a");
                assert(fp != NULL);
                this->path = temp;
                break;
            }
        }
    }
    return true;
}


void* Log::strike(void * args) 
{
    Log::get_instance()->worker();
    return NULL;
}


bool Log::log(string type, const char* format, ...) 
{
    //添加任务
    if (task_num > MAX_TASK_NUM) return false;
    va_list valst;
    va_start(valst, format);
    char buf[100];
    vsnprintf(buf, 100, format, valst);
    va_end(valst);
    LogTask new_task;
    new_task.type = type;
    new_task.content = buf;
    mutex.lock();
    task_queue.push(new_task);
    task_num++;
    mutex.unlock();
    sem.post();
    return true;
}


void Log::worker()
{
    //工作线程
    while(true) 
    {
        sem.wait();
        if (task_queue.empty()) continue;
        stat(path.c_str(), &file_stat);
        if (file_stat.st_size > MAX_LOG_FILE) 
        {
            fclose(fp);
            get_new_fp();
        } 
        //prepare
        std::string day, second, cur_path;
        time_t t = time(NULL);
        cur = *localtime(&t);
        mutex.lock();
        struct LogTask task = task_queue.front();
        task_queue.pop();
        task_num--;
        mutex.unlock();
        //get second
        char tmp[20];
        snprintf(tmp, 20, "%02d.%02d.%02d", cur.tm_hour, cur.tm_min, cur.tm_sec);
        second = tmp;
        //write
        std::string write_content = second + "[" + task.type + "]:" + task.content + "\n";
        fputs(write_content.c_str(), fp);  
        fflush(fp);
    }
}
