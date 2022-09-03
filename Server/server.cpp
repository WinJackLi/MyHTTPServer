#include "./server.h"


Server::Server()  {
    std::cout<<"服务器启动..."<<std::endl;
    stop_server = false;
}


Server::~Server(){
    close(serv_sock);       
    close(epoll);
    sql_pool->destroy_pool();   
    pool->destroy_pool();
    delete timer;      
    delete serve;
    delete [] events;
    delete [] user;
	std::cout<<"服务器已关闭";
}


void Server::init(int port, const char* path, int sql_port, string sql_url, string sql_user, string sql_database, string sql_passwd) {
    std::cout<<"端口号为:"<<port<<std::endl;
    this->port = port;
    sock_init();  //服务端套接字
    epoll_init(); //epoll例程
    timer = new Timer(clnt_num, epoll);
    timer->start(); //启动定时器服务
    
    //用户数据表
    user = new User[MAX_CLNTS];

    //异步日志系统启动
    Log::get_instance()->start();   //异步日志系统启动

    //使用单例模式创建数据库连接池
    sql_pool = Connectpool::get_instance(); 
    sql_pool->init(sql_port, sql_url, sql_user, sql_passwd, sql_database);

    //初始化http服务程序
    serve = new http_serve(clnt_num);
    serve->init(epoll, path, user, timer, sql_pool);

    //初始化线程池
    pool = Threadpool::get_instance();
    pool->init(epoll, user, timer, sql_pool, serve);
}


void Server::sock_init(){
    //初始化服务端套接字
    struct sockaddr_in serv_adr;
    socklen_t adr_sz;
    
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(port);

    //no time-wait
    int option = 1; 
    socklen_t optlen = sizeof(option);
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&option, optlen);

    assert(bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) != -1);
    assert(listen(serv_sock, 5) != -1);
    return ;

}


void Server::epoll_init(){
    //初始化epoll，边缘触发

    epoll = epoll_create(5);
    assert(epoll != -1);
	events = new epoll_event[EPOLL_SIZE];

    struct epoll_event event;
    epoll_add(serv_sock, false, 1);          
    
    //信号，统一事件源
    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipe);  //用于与定时器交流的管道
    assert(ret != -1);
    pipe_fd = pipe[1];
    setnonblocking(pipe[1]);
    epoll_add(pipe[0], false, 0); //LT模式同步处理
    addsig(SIGINT);
    addsig(SIGALRM);
    addsig(SIGHUP);
    addsig(SIGTERM);   
}


void Server::epoll_add( int fd, bool one_shot, int mode)
{
    epoll_event event;
    event.data.fd = fd;
    if (one_shot)
        event.events |= EPOLLONESHOT;
    if (1 == mode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;
    epoll_ctl(epoll, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}


int Server::accept_conn_request()
{
    //处理客户端连接请求
    
    struct sockaddr_in clnt_adr;
    socklen_t adr_sz = sizeof(clnt_adr);
    int clnt_sock;
    while (1) 
    {
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &adr_sz);    
        if (clnt_sock < 0) {
            return -1;
        }
        if (clnt_num >= MAX_CLNTS) {
            LOG("warn", "server::accept_conn_request::too much client!");
            return -1;
        }
        assert(clnt_sock < MAX_CLNTS);
        epoll_add(clnt_sock, true, 1);
        timer->sign_in(clnt_sock);
        clnt_num++;
        LOG("info","Connect fd %d", clnt_sock);
    }
    return 0;
}


bool Server::deal_signal(bool &timeout, bool &stop_server)
{
    int ret = 0;
    int sig;
    char signals[24];
    ret = recv(pipe[0], signals, sizeof(signals), 0);
    if (ret == -1)
        return false;
    else if (ret == 0)
        return false;
    else {
        for (int i = 0; i < ret; ++i) 
        {
            switch (signals[i])
            {
                case SIGALRM:
                {
                    timeout = true;
                    break;
                }
                case SIGHUP:
                {
                    break;
                }
                case SIGTERM:
                case SIGINT:
                {
                    stop_server = true;
                    break;
                }
            }
        }
    }
    return true;
}


void Server::event_loop()
{
    //循环处理事件
    int event_num;
    int i, fd;
    while (!stop_server) {
        event_num = epoll_wait(epoll, events, EPOLL_SIZE, -1);
        assert(event_num != 0);
        for (int i = 0; i < event_num; i++) 
        {
            fd = events[i].data.fd;
            if (fd == serv_sock) 
            {
                accept_conn_request(); 
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                timer->log_out(fd);
                epoll_del(epoll, fd);
                --clnt_num;
            }
            else if ((fd == pipe[0]) && (events[i].events & EPOLLIN))
            {
                bool ret = deal_signal(timeout, stop_server);
                if (false == ret) 
                    LOG("error", "deal signal fail");
            }
            else if (events[i].events & EPOLLIN) {
                user[fd].type = READABLE;
                pool->add_task(fd);
                //Reactor模式
                while (true) {
                    if (user[fd].over) 
                    {
                        if (user[fd].close_fd) {
                            timer->log_out(fd);
                            epoll_del(epoll, fd);
                            --clnt_num;
                            user[fd].close_fd = false;
                        }
                        else {
                            timer->update(fd);
                        }
                        user[fd].over = false;
                        break;
                    }
                }               

            }   
            else if (events[i].events & EPOLLOUT) {
                user[fd].type = WRITABLE;
                pool->add_task(fd);
                while (true) {
                    if (user[fd].over) 
                    {
                        if (user[fd].close_fd) {
                            timer->log_out(fd);
                            epoll_del(epoll, fd);
                            --clnt_num;
                            user[fd].close_fd = false;
                        }
                        else {
                            timer->update(fd);
                        }
                        user[fd].over = false;
                        break;
                    }
                }
            }
        }
        if (timeout)
        {
            timer->tick();
            timeout = false;
        }
    }
}
