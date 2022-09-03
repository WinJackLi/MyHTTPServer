#ifndef PARSER_H
#define PARSER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "./state.h"
#include "../User/user.h"
#include "../Log/log.h"


#define BUFFER_SIZE 4096

class Parser {
    char* get_tar_next(char* str , const char *target); 
    HTTP_CODE parse_method(char *str);

public:
    User* user;
    LINE_STATUS parse_line(char* buf, int& parser_idx, int& read_idx );
    HTTP_CODE start_parse(int fd, char *buf, int read_idx, int& parser_idx, int& line_start_idx); 
    HTTP_CODE parse_requestline(int fd, char* line);
    HTTP_CODE parse_headers(int fd, char* line);
    HTTP_CODE parse_content(int fd, char* line, int parser_idx, int read_idx);
};
#endif