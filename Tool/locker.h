#ifndef LOCKER_H
#define LOCKER_H

#include <thread>
#include <semaphore.h>
#include "tool.h"
#define READER 0 
#define WRITER 1 

using namespace std;


class Sem {
    //信号量
    sem_t sem;
    int val;
public:
    Sem(int v = 0): val(v) {
        if (sem_init(&sem, 0, val)) {
            perr("sem_init");
        } 
    }
    ~Sem() {
        sem_destroy(&sem);
    }
    int wait() {
        return sem_wait(&sem);
    }
    int post() {
        return sem_post(&sem);
    }
};


class Cond {
    //条件变量
    pthread_cond_t cond;
    pthread_mutex_t mutex;
public:
    Cond() {
        if (pthread_mutex_init(&mutex, nullptr) != 0) {
            perr("mutex init for condition");
        }
        if (pthread_cond_init(&cond, nullptr) != 0) {
            pthread_mutex_destroy(&mutex);
            perr("cond init");
        }
    }
    ~Cond() {
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }
    int wait() {
        //等待条件变量
        int ret = 1; 
        pthread_mutex_lock(&mutex);
        ret = pthread_cond_wait(&cond, &mutex);
        pthread_mutex_unlock(&mutex);
        return ret;
    }
    int signal() {
        //唤醒一个以上等待目标条件变量的线程
        return pthread_cond_signal(&cond);
    }

    int broadcast() {
        //广播式唤醒所有等待目标条件变量的线程
        return pthread_cond_broadcast(&cond);
    }
};


class Mutexlock {
    //嵌套互斥锁
    pthread_mutex_t mutex;
    pthread_mutexattr_t attr;
public:
    Mutexlock() {
        if (pthread_mutexattr_init(&attr)) {
            perr("mutex attr init");
        }
        if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE)){
            perr("mutex attr set");
        }
        if (pthread_mutex_init(&mutex, &attr)) {
            perr("mutex init");
        }
        if (pthread_mutexattr_destroy(&attr)) {
            perr("mutex attr destory");
        }
    }
    ~Mutexlock() {
        pthread_mutex_destroy(&mutex);
    }
    int lock() {
        return pthread_mutex_lock(&mutex);
    }
    int unlock() {
        return pthread_mutex_unlock(&mutex);
    }
};


class Spinlock {
    //自旋锁
    pthread_spinlock_t spinlock;
public:
	Spinlock() {
		//PTHREAD_PROCESS_PRIVATE，只能被初始化该锁的进程内部的线程访问
		//PTHREAD_PROCESS_SHARED，进程共享的
		pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE);
	}
	~Spinlock() {
		pthread_spin_destroy(&spinlock);
	}
	int lock() {
		return pthread_spin_lock(&spinlock);
	}
	int trylock() {
        //非阻塞(不自旋),不能获取锁则立即返回EBUSY错误，可以设置循环模拟lock()
		return pthread_spin_trylock(&spinlock);
	}
	int unlock() {
		return pthread_spin_unlock(&spinlock);
	}
};


class Rwlock {
    //读写锁，传入->WRITER: 写者优先
    pthread_rwlockattr_t attr;
    pthread_rwlock_t rwlock;
public:  
    Rwlock(int prefer = READER){
        if (pthread_rwlockattr_init(&attr) != 0) {
            perr("rwlock attr init");
        }
        if (prefer != READER) {
            if (pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NP) != 0 ){
                perr("rwlock attr set");
            }   
            if (pthread_rwlockattr_destroy(&attr) != 0) {
                perr("rwlock attr destory");
            }
        }
        if (pthread_rwlock_init(&rwlock, &attr) != 0) {
            perr("rwlock init");
        }
    }
    ~Rwlock(){
        if (pthread_rwlock_destroy(&rwlock) != 0) {
            perr("rwlock destory");
        }
    }
    int rdlock() {
        return pthread_rwlock_rdlock(&rwlock);
    }
    int wrlock() {
        return pthread_rwlock_wrlock(&rwlock);
    }
    int unlock() {
        return pthread_rwlock_unlock(&rwlock); 
    }
};


#endif