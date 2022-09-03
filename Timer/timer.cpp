#include "./timer.h"



void cb_func(int epoll, int fd)
{
    epoll_del(epoll, fd);
}


Timer::Timer(int& clnt_num, int epoll ):
rear(0), clnt_num(clnt_num), epoll(epoll) {    
    heap[rear] = NULL;
}


Timer::~Timer() {}


bool Timer::heap_add(client *clnt) {
    if (rear < MAX_CLNTS) {
        int j = ++rear; //子结点
        int i = rear/2; //父结点
        heap[rear] = clnt;
        heap[0] = heap[rear];
        for (i; i > 0; i /= 2) {
            if (heap[i]->timeout <= heap[0]->timeout) {
                break;
            }
            else {
                heap[j] = heap[i];
                heap[j]->id = j;
                j = i;
            }
        }
        heap[j] = heap[0];
        heap[j]->id = j;
        return true;
    }
    else {
        LOG("error", "Too much client");
        return false;
    }
}


bool Timer::heap_del() {
    if (rear > 0) {
        heap[1] = heap[rear--];
        heap[1]->id = 1;
        //调整堆
        heap[0] = heap[1]; 
        int i = 1;
        int j = i*2;
        for (j; j <= rear; j *= 2) {
            if (j < rear && heap[j]->timeout > heap[j+1]->timeout)
                j++;
            if (heap[0]->timeout <= heap[j]->timeout) {
                break;
            }
            else {
                heap[i] = heap[j]; //这里不是换值
                heap[i]->id = i;
                i = j; //i为新根节点
            }
        }
        heap[i] = heap[0]; //i是最终的位置
        heap[i]->id = i;
        return true;
    }
    else {
        return false;
        LOG("error", "timer::heap_del():: No client to del");
    }
}


bool Timer::clean_timer(client* clnt) {
    if (clnt) {
        clnt->fd = 0;
        clnt->id = 0;
        clnt->timeout = 0;
        clnt->cb_func = NULL;
        return true;
    }
    else {
        return false;   
    }
}


bool Timer::heap_del(int id) {
    //根据位置id删除定时器
    if (rear > 0) {
        if (id == rear) {
            clean_timer(heap[id]);
            rear--;
            return true;
        }
        heap[id] = heap[rear--];
        heap[id]->id = id;
        //调整堆，下滤
        heap[0] = heap[id]; 
        int i = id;
        int j = i*2;
        for (j; j <= rear; j *= 2) {
            if (j < rear && heap[j]->timeout > heap[j+1]->timeout)
                j++;
            if (heap[0]->timeout <= heap[j]->timeout) {
                break;
            }
            else {
                heap[i] = heap[j]; //这里不是换值
                heap[i]->id = i;
                i = j; //i为新根节点
            }
        }
        heap[i] = heap[0]; //i是最终的位置
        heap[i]->id = i;
        return true;
    }
    else {
        return false;
        LOG("error", "timer::heap_del():: No client to del");
    }
}


void Timer::heap_adjust(int i){
    //下滤掉小的时限
    heap[0] = heap[i];
    for (int j = i*2; j <= rear; j *= 2) {
        if (j < rear && heap[j]->timeout > heap[j+1]->timeout) {
            j++;
        }
        if (heap[0]->timeout <= heap[j]->timeout) {
            break;
        }
        else {
            heap[i] = heap[j];
            heap[i]->id = i;
            i = j;
        }
    }
    heap[i] = heap[0];
    heap[i]->id = i;
}


void Timer::tick() {
    time_t present = time(NULL);
    if (rear) {
        //遍历每一个根节点，调整最小堆
        for (int i = rear/2; i > 0; i--) {
            heap_adjust(i);
        }
        //删除超时定时器，关闭连接
        while (rear > 0 && heap[1]->timeout <= present) {
            LOG("info", "tick : remove fd %d", heap[1]->fd);
            if (heap[1]->cb_func){
                heap[1]->cb_func(epoll, heap[1]->fd);   
            }
            if (heap_del()) {
                clnt_num--;
            } 
        }
        if (rear) {
            alarm(heap[1]->timeout-present); //下个间隔
        }
        else {
            LOG("info", "tick ...");
            alarm(SHORT_TICK);
        }
    }
    else {
        LOG("info", "tick ...");
        alarm(SHORT_TICK); //每五秒启动一次，避免阻塞程序
    }
}


bool Timer::sign_in(int fd, int timeout) {
    
    clients[fd].fd = fd; //维护地址信息
    clients[fd].timeout = timeout + time(NULL);
    clients[fd].cb_func = cb_func;
    return heap_add(&(clients[fd]));
}


bool Timer::log_out(int fd) {
    if(fd < MAX_CLNTS) {
        heap_del(clients[fd].id);
        return true;
    }
    return false;
}


bool Timer::update(int fd, int timeout) {
    //更新fd的时限
    if (fd < MAX_CLNTS) {
        clients[fd].timeout = timeout;
        clients[fd].cb_func = cb_func;
        return true;
    }
    return false;
}


void Timer::start() {
    //启动定时器
    alarm(SHORT_TICK);
}

