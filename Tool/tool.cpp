#include "./tool.h"

int pipe_fd;

void addsig(int sig) {
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
    return ;
}

void sig_handler( int sig )
{
    int save_errno = errno;
    int msg = sig;
    send(pipe_fd, ( char* )&msg, 1, 0 );
    errno = save_errno;
}


//非阻塞fd,返回旧状态
int setnonblocking( int fd ) {
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}

//输出错误
void perr(const char* the_error, const char* file_name, int line) {
    std::cout<<">>> ERROR : "<<the_error<<std::endl;
    if (file_name) std::cout<<">>> filename : "<<file_name<<std::endl;
    if (line) std::cout<<">>> line : "<<line<<std::endl;
    exit(0);
}

void epoll_del(int epoll, int fd) {
    epoll_ctl(epoll, EPOLL_CTL_DEL,fd, 0);
    close(fd);
}