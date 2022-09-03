#ifndef USER_H
#define USER_H

#include <string>
#include <stddef.h>
#include <sys/mman.h>
#include "../config.h"
#include "../HTTP/state.h"

struct User {
    int type;                         //READABLE||WRITABLE
    bool over;                        //读取fd信息结束
    bool close_fd;                    //读取fd信息时发生错误
    bool keep_alive;                  //是否保持连接
    int read_idx;                     //读取信息的指针，指向第一个空字符，可代表已读内容长度
    int parser_idx;                   //解析信息的指针
    int start_line_idx;               //解析出的行的头指针
    int write_idx;                    //写响应的指针，指向第一个空字符，可代表已写内容长度
    int content_len;                  //post表单的内容长度
    char* content;                    //post表单的内容指针
    std::string protocol;             //HTTP/1.1||HTTP/1.0
    char read_buf[READ_BUF_SIZE];     //读取缓冲

    char write_buf[WRITE_BUF_SIZE];   //写缓冲
    char* map_buf;                    //文件映射的内存空间需要手动释放
    size_t map_len;                   //文件映射的内存空间大小
    struct iovec iv[2];               //调用load_file载入文件内存指针，提供给writev
    int iv_count;                     //记录带发送内存块的数量
    size_t bytes_to_send;             //未发送的字节数
    size_t bytes_have_send;           //已发送的字节数

    char *url;                        //客户请求的url
    HTTP_CODE method;                 //http请求的类型(get||post||bad||...)
    CHECK_STATE check_state;          //解析状态(头部||请求行||内容)

public:

    User() {
        type = READABLE;
        over = false;
        close_fd = false;
        keep_alive = false;
        url = NULL;
        read_idx = 0;
        parser_idx = 0;
        start_line_idx = 0;
        write_idx = 0;
        content_len = 0;         
        content = NULL;
        iv[0].iov_len = 0;
        iv[0].iov_base = NULL;
        iv[1].iov_len = 0;
        iv[1].iov_base = NULL;
        iv_count = 0;
        bytes_to_send = 0;
        bytes_have_send = 0;
        method = REQUEST_GET;
        check_state = CHECK_STATE_REQUESTLINE;
    }

    void clean() {
        type = READABLE;
        keep_alive = false;
        url = NULL;
        read_idx = 0;
        parser_idx = 0;
        start_line_idx = 0;
        write_idx = 0;
        content_len = 0;         
        content = NULL;
        iv[0].iov_len = 0;
        iv[0].iov_base = NULL;
        iv[1].iov_len = 0;
        iv[1].iov_base = NULL;
        iv_count = 0;
        bytes_to_send = 0;
        bytes_have_send = 0;
        method = REQUEST_GET;
        check_state = CHECK_STATE_REQUESTLINE;
        munmap(map_buf, map_len);
    }

};

#endif