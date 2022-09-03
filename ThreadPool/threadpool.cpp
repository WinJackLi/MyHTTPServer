#include "./threadpool.h"

Threadpool* Threadpool::get_instance() {
    static Threadpool threadpool;
    return &threadpool;
}


void Threadpool::destroy_pool() {
    mutexlock.lock();
    close_pool = true;               //工作完线程的自动退出
    LOG("info","threadpool::destroy_pool");
    mutexlock.unlock();
}   


Threadpool::Threadpool() {
    pool_size = 0;
    max_pool_size = MAX_POOL_SIZE;
    task_num = 0;
    alive_thread = 0;
    dead_thread = 0;
    working_thread = 0;
    close_pool = false;
}


Threadpool::~Threadpool() {
    destroy_pool();
}


void Threadpool::init( int epoll, User* user, Timer* timer, Connectpool* sql_pool, http_serve* serve) {
    this->user = user;
    this->timer = timer;
    this->sql_pool = sql_pool;
    this->epoll = epoll;
    this->serve = serve;
    /*
    if(pthread_create(&(manager_thread.id), NULL, manager_start, this)) {
        perr("threadpool::init::pthread_create");
    }
    if(pthread_detach(manager_thread.id)) {
        perr("threadpool::init::pthread_detach");
    }
    */
    for ( int i = 0; i < POOL_SIZE; ++i ) {
        if (add_thread()) {
            perr("threadpool::init->add_thread()",NULL, 73);
        }
    }
}


void Threadpool::add_task(int fd) {
    mutexlock.lock();
    task_queue.push(fd);
    ++task_num;
    mutexlock.unlock();
    sem.post();    
}


int Threadpool::add_thread() {
    //增加存活线程
    Thread newthread; 
    if (pthread_create(&newthread.id, NULL, start, this)) {
        return PTHREAD_CREATE_FAILED;
    }
    if (pthread_detach(newthread.id)) {
        return PTHREAD_DETCH_FAILED;
    }
    /*
    if (dead_thread) {
        for (int j = 0; j < pool_size; ++j) {
            if (pool[j].state = DEAD) {
                pool[j] = newthread;
                --dead_thread;
                ++alive_thread;
                break;
            }
        }
    }
    else {*/
        pool.push_back(newthread);
        ++alive_thread;
        ++pool_size;
    //}
    return 0;
}


/*
int Threadpool::del_thread() {
    //销毁一条Alive线程，因为没有执行代码，因此cancel安全
    for (int i = 0; i < pool_size; ++i) {
        if (pool[i].state == ALIVE) {
            if (pthread_cancel(pool[i].id)) {
                return PTHREAD_CANCEL_FAILED;
            }
            pool[i].state = DEAD;
            ++dead_thread;
            --alive_thread;
            return 0;
        }
    }
    return PTHREAD_DELETE_FAILED;
}

void* Threadpool::manager_start( void* arg ) {
    //启动管理线程
    Threadpool* ptr = (Threadpool *)arg;
    ptr->manager_worker();
    return NULL;
}


void Threadpool::manager_worker() {
    while(1) {
        //调整容量
        sleep(TIME_TO_SLEEP);
        mutexlock.lock();
        int num = alive_thread + working_thread;
        if (task_num > num * TIME_TO_EXPAND) {
            for (int i = 0; i < (max_pool_size - num); ++i ) {
                add_thread();
            }
        }
        else if (task_num > num * TIME_TO_ADD) {
            for (int i = 0; i < (pool_size - num); ++i) {
                add_thread();
            } 
        }
        else if (task_num < num / TIME_TO_DEL) {
            for (int i = 0; i < (pool_size - POOL_SIZE); ++i) {
                del_thread();
            }
        }
        LOG("info", "threadpool::manager_worker::thread number = %d", alive_thread + working_thread);
        mutexlock.unlock();
    }
}
*/
void* Threadpool::start( void* arg ) {
    //启动工作线程
    Threadpool* ptr = ( Threadpool* )arg;
    ptr->worker();
    return NULL;
}


int Threadpool::get_task() {
    //获取一个任务
    while (1) {
        sem.wait();
        mutexlock.lock();
        if (task_queue.empty()) {  //空队列
            mutexlock.unlock();
            continue;
        }
        else {
            int fd = task_queue.front();
            task_queue.pop();
            task_num--;
            mutexlock.unlock();
            return fd;
        }
    }
}


void Threadpool::worker() {
    /*
    //工作线程，先获取自身在线程池中的位置
    int index, fd;
    pthread_t self_id = pthread_self();
    mutexlock.lock();
    for (int i = 0; i < pool_size; ++i) {
        if (pool[i].id == self_id && pool[i].state == ALIVE ) {
            index = i; 
            break;
        }
    }
    //开始工作
    LOG("info", "threadpool::worker::thread:%u start to work", self_id);
    mutexlock.unlock();
    */
    int fd;
    while (!close_pool ) {   
        //pool[index].state = ALIVE;
        fd = get_task();
        //pool[index].state = WORKING;
        if (user[fd].type == READABLE) 
        {
            mutexlock.lock();
            if(serve->read_out(fd)) 
            {
                user[fd].over = true;
                serve->process(fd);
            }
            else
            {
                //处理request失败 : 发生错误 || 非法数据(缓冲区满) || 已经断开连接
                //发送数据 : 发送完成且!keep_alive || 出现错误 || 连接断开
                user[fd].over = true;
                user[fd].close_fd = true;
            }
            mutexlock.unlock();

        }
        if (user[fd].type == WRITABLE) 
        {
            mutexlock.lock();
            if (serve->write_in(fd)) {
                user[fd].over = true;
            }
            else {
                user[fd].over = true;
                user[fd].close_fd = true;
            }
            mutexlock.unlock();
        }
    }
}
