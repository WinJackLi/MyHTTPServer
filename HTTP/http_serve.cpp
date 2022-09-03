#include "./http_serve.h"

http_serve::http_serve(int& clnt_num):clnt_num(clnt_num) {}
http_serve::~http_serve() 
{
    for (auto it = files.begin(); it != files.end(); it++) {
        munmap(it->second.addr, it->second.size);
    }
}


void http_serve::init(int epoll, const char* path, User* user, Timer* timer, Connectpool* sql_pool)
{
    //初始化服务程序
    this->user = user;
    this->epoll = epoll;
    this->timer = timer;
    this->sql_pool = sql_pool;
    this->path = path;
    parser.user = user;
    sql_get_data(); //先从mysql中缓存数据
    file_prepare(); //准备好HTML文件
}


void http_serve::process(int fd) 
{
    int ret = deal_request(fd);
    if (ret == -1) {
        epoll_del(epoll, fd);
        mutexlock.lock();
        clnt_num--;
        mutexlock.unlock();
        return;
    }
    if (ret == 0) {
        mod_fd(epoll, fd, EPOLLOUT); //当缓冲区空闲触发
        return;
    } 
    if (ret == 1) {
        mod_fd(epoll, fd, EPOLLIN);
        return;
    }
}


void http_serve::sql_get_data() 
{
    //先从连接池中取一个连接
    MYSQL* mysql = sql_pool->get_conn();
    //在user表中检索username，passwd数据，浏览器端输入
    if (mysql_query(mysql, "select name, email, password from user_data")) 
    {
        perr("can't get user data");
    }
    //从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);

    //返回结果集中的列数
    int num_fields = mysql_num_fields(result);

    //返回所有字段结构的数组
    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    //从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result)) 
    {
        Data data(row[2], row[3]);
        this->data[row[0]] = data;
    }
    sql_pool->release_conn(mysql);
}


void http_serve::add_file(const char* name , int limit) 
{
    //limit == 1时修改为保护模式，用户不能通过get直接访问，默认公开
    int fd;
    char file_path[PATH_SIZE] = {0};
    snprintf(file_path, PATH_SIZE, "%s%s", path, name);
    struct stat file_stat;
    assert(stat(file_path, &(file_stat)) >= 0);
    //修改文件权限
    if (limit)  chmod(file_path, S_IRWXU);
    fd = open(file_path, O_RDONLY);
    assert(fd > 0);
    //映射并添加到文件夹
    char* addr = (char*)mmap(0, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    LOG("info", "At %p add file %s ", addr , name);
    file new_file(name, file_stat.st_size, addr);
    files[string(name)] = new_file;
    close(fd);
}


void http_serve::file_prepare() 
{
    //准备一些待发送文件，提高速度
    add_file("welcome.html");
    add_file("main.html", 1);
    add_file("bad_email.html");
    add_file("bad_name.html");
    add_file("bad_password.html");
    add_file("repeat_name.html");
}


void http_serve::iov_prepare(int fd, size_t size1, char* base1, size_t size2, char* base2)
{
    if (size1) {
        user[fd].iv[0].iov_len = size1;
        user[fd].iv[0].iov_base = base1;
        user[fd].bytes_to_send = size1;
        user[fd].iv_count = 1;
    }
    if (size2) {
        user[fd].map_buf = base2;
        user[fd].iv[1].iov_len = size2;
        user[fd].iv[1].iov_base = base2;
        user[fd].bytes_to_send += size2;
        user[fd].iv_count = 2;
    }
}


int http_serve::deal_request(int fd) 
{
    //最重要的逻辑处理函数
    HTTP_CODE retcode;
    bool ret;
    retcode = parser.start_parse(fd, user[fd].read_buf, user[fd].read_idx, user[fd].parser_idx, user[fd].start_line_idx);    
    //业务处理
    switch (retcode)
    {
        case REQUEST_GET: 
        {
            if (!strncasecmp(user[fd].url, "welcome.html",12)) 
            {
                ret = add_response(fd, 200, ok_200_title, files["welcome.html"].size, "text/html", "UTF-8", user[fd].keep_alive);
                iov_prepare(fd, user[fd].write_idx, user[fd].write_buf, files["welcome.html"].size, files["welcome.html"].addr);
                break;
            }
            else {
                char req_path[PATH_SIZE] = {0};
                snprintf(req_path, PATH_SIZE, "%s%s", path, user[fd].url); 
                struct stat file_stat;
                if( stat(req_path, &file_stat) < 0 ) 
                {
                    ret = add_response(fd, 404, error_404_title, 0, "text/html", "UTF-8", user[fd].keep_alive);     
                    iov_prepare(fd, user[fd].write_idx, user[fd].write_buf, 0, NULL);       
                    break;       
                }
                if (!(file_stat.st_mode&S_IROTH))
                {
                    ret = add_response(fd, 403, error_403_title, 0, "text/html", "UTF-8", user[fd].keep_alive);     
                    iov_prepare(fd, user[fd].write_idx, user[fd].write_buf, 0, NULL);              
                    break;
                }
                if (S_ISDIR(file_stat.st_mode)) 
                {
                    //是一个文件夹
                    ret = add_response(fd, 400, error_400_title, 0, "text/html", "UTF-8", user[fd].keep_alive);     
                    iov_prepare(fd, user[fd].write_idx, user[fd].write_buf, 0, NULL);              
                    break;
                }
                std::string name(user[fd].url);
                if (files.find(name) == files.end()) {
                    //添加到文件夹中
                    add_file(name.c_str());
                }
                else {
                    if (files[name].size != file_stat.st_size) {
                        //文件发生了更改
                        files.erase(name);
                        add_file(name.c_str());
                    }
                }
                user[fd].map_buf = files[name].addr;
                ret = add_response(fd, 200, ok_200_title, file_stat.st_size, NULL, NULL, user[fd].keep_alive);
                iov_prepare(fd, user[fd].write_idx, user[fd].write_buf, file_stat.st_size, user[fd].map_buf);
            }
            break;
        }
        case REQUEST_POST: 
        {
            //提取出post表单内容
            char name[20], email[30], password[30];
            int i;
            for (i = 5; user[fd].content[i] != '&' && i < 25; ++i){
                name[i - 5] = user[fd].content[i];
            }
            name[i - 5] = '\0';
            int j = 0;
            for (i = i + 7; user[fd].content[i] != '&' && j < 30; ++i, ++j){
                email[j] = user[fd].content[i];
            }
            email[j] = '\0';
            int k = 0;
            for (i = i + 10; user[fd].content[i] != '\0' && k < 30; ++i, ++k) {
                password[k] = user[fd].content[i];
            }
            password[k] = '\0';
            //登陆
            if (!strncasecmp(user[fd].url, "sign_in.html", 12)) 
            {
                if (data.find(name) == data.end()) 
                {
                    ret = add_response(fd, 400, error_400_title, files["bad_name.html"].size, "text/html", "UTF-8", user[fd].keep_alive);
                    iov_prepare(fd, user[fd].write_idx, user[fd].write_buf, files["bad_name.html"].size, files["bad_name.html"].addr);
                }
                else {
                    if (data[name].email == email) 
                    {   
                        //邮箱验证失败
                        ret = add_response(fd, 400, error_400_title, files["bad_email.html"].size, "text/html", "UTF-8", user[fd].keep_alive);
                        iov_prepare(fd, user[fd].write_idx, user[fd].write_buf, files["bad_email.html"].size, files["bad_email.html"].addr);
                        break;
                    }
                    if (data[name].password == password)
                    {   
                        //密码验证失败
                        ret = add_response(fd, 400, error_400_title, files["bad_password.html"].size, "text/html", "UTF-8", user[fd].keep_alive);
                        iov_prepare(fd, user[fd].write_idx, user[fd].write_buf, files["bad_password.html"].size, files["bad_password.html"].addr);
                        break;
                    } 
                    //验证通过
                    ret = add_response(fd, 200, ok_200_title, files["main.html"].size, "text/html", "UTF-8", user[fd].keep_alive);
                    iov_prepare(fd, user[fd].write_idx, user[fd].write_buf, files["main.html"].size, files["main.html"].addr);
                }
                break;             
            }
            //注册
            if (!strncasecmp(user[fd].url, "sign_up.html", 12)) 
            {
                if (data.find(name) != data.end()) 
                {   
                    //重复
                    ret = add_response(fd, 400, error_400_title, files["repeat_name.html"].size, "text/html", "UTF-8", user[fd].keep_alive);
                    iov_prepare(fd, user[fd].write_idx, user[fd].write_buf, files["repeat_name.html"].size, files["repeat_name.html"].addr);
                }
                else 
                {
                    MYSQL* mysql = sql_pool->get_conn();
                    char statement[200];
                    Data new_data(email, password);
                    snprintf(statement, 200, "insert into user_data(name, email, password) values('%s', '%s', '%s');", name, email, password);
                    mutexlock.lock();
                    mysql_query(mysql, statement);
                    data.insert(pair<string, Data>(name, new_data));
                    mutexlock.unlock();
                    sql_pool->release_conn(mysql);
                    //写响应
                    ret = add_response(fd, 200, ok_200_title, files["main.html"].size, "text/html", "UTF-8", user[fd].keep_alive);
                    iov_prepare(fd, user[fd].write_idx, user[fd].write_buf, files["main.html"].size, files["main.html"].addr);
                }
                break;             
            }

        }
        case REQUEST_KPON:
        {
            return 1;
        }
        case REQUEST_BAD: 
        {
            ret = add_response(fd, 400, error_400_title, 0, "text/html", "UTF-8", user[fd].keep_alive);
            iov_prepare(fd, user[fd].write_idx, user[fd].write_buf, 0, NULL);   
            break;
        }
        default: 
        {
            ret = add_response(fd, 500, error_500_title, 0, "text/html", "UTF-8", user[fd].keep_alive);
            iov_prepare(fd, user[fd].write_idx, user[fd].write_buf, 0, NULL);   
            break;
        }
    }
    if (ret)
        return 0;
    else 
        return -1;
}


bool http_serve::read_out(int fd) 
{
    //ET非阻塞模式下读出客户端信息
    int& read_idx = user[fd].read_idx;
    char* buf = user[fd].read_buf;

    if (read_idx >= READ_BUF_SIZE ){
        return false;
    }
    int ret = 0;
    while (true) {
        ret = recv(fd, buf+read_idx, READ_BUF_SIZE - read_idx,  0);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; //读缓冲读完了
            }
            return false; //发生错误
        }
        if (ret == 0) {
            return false; //断开连接或者缓冲区读满了(非法数据)
        }
        read_idx += ret;
    }
    return true;
}


bool http_serve::write_in(int fd) 
{
    int temp = 0;
    while (user[fd].bytes_to_send > 0)
    {
        temp = writev(fd, user[fd].iv, user[fd].iv_count);
        if (temp < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                mod_fd(epoll, fd, EPOLLOUT);
                return true;
            }
            LOG("error", "fd:%d when writev : %s", fd, strerror(errno));
            user[fd].clean();  
            return false;   //出错，关闭该连接
        }

        user[fd].bytes_have_send += temp;
        user[fd].bytes_to_send -= temp;

        if (user[fd].bytes_have_send >= user[fd].iv[0].iov_len)
        {
            user[fd].iv[0].iov_len = 0;
            user[fd].iv[1].iov_base = user[fd].map_buf + (user[fd].bytes_have_send - user[fd].write_idx);
            user[fd].iv[1].iov_len = user[fd].bytes_to_send ;
        }
        else
        {
            user[fd].iv[0].iov_base = user[fd].write_buf + user[fd].bytes_have_send;
            user[fd].iv[0].iov_len = user[fd].iv[0].iov_len - user[fd].bytes_have_send;
        }
    }
    if (user[fd].keep_alive)
    {
        user[fd].clean();   //上面也return true，但是不需要清空fd，因此这里清空
        mod_fd(epoll, fd, EPOLLIN);
        return true;
    }
    else {
        user[fd].clean();  
        return false;   //关闭该连接
    }
}


bool http_serve::add_response(int fd, int status, const char* title, int content_length, 
                                const char* content_type, const char* charset, bool keep_alive) 
{
    //添加响应头
    if (user[fd].write_idx >= WRITE_BUF_SIZE) return false;
    int& idx = user[fd].write_idx;
    char* buf = user[fd].write_buf;
    int len;

    len = snprintf(buf + idx, WRITE_BUF_SIZE-idx-1, "%s %d %s\r\n", user[fd].protocol.c_str(), status, title);
    if (len >= WRITE_BUF_SIZE-idx-1) return false;
    idx += len;

    len = snprintf(buf + idx, WRITE_BUF_SIZE-idx-1, "Content-Length:%d\r\n", content_length);
    if (len >= WRITE_BUF_SIZE-idx-1)  return false;
    idx += len;
    
    if (content_type && charset) {
        len = snprintf(buf + idx, WRITE_BUF_SIZE-idx-1, "Content-Type:%s; charset=%s\r\n", content_type, charset);
        if (len >= WRITE_BUF_SIZE-idx-1)  return false;
        idx += len;
    }

    len = snprintf(buf + idx, WRITE_BUF_SIZE-idx-1, "Connection:%s\r\n",(keep_alive == true)? "keep-alive":"close");
    if (len >= WRITE_BUF_SIZE-idx-1)  return false;
    idx += len;

    len = snprintf(buf + idx, WRITE_BUF_SIZE-idx-1, "%s", "\r\n");
    if (len >= WRITE_BUF_SIZE-idx-1)  return false;
    idx += len;
    return true;
}


void http_serve::mod_fd(int epoll, int fd, int events) 
{
    //结束服务后重新设置fd为OneShot,并且重新注册事件
    epoll_event event;
    event.data.fd = fd;
    event.events = events|EPOLLET|EPOLLONESHOT|EPOLLRDHUP;
    epoll_ctl(epoll, EPOLL_CTL_MOD, fd, &event);
}
