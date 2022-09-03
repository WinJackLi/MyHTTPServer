#ifndef STATE_H
#define STATE_H

enum CHECK_STATE { 
    CHECK_STATE_REQUESTLINE = 0, 
    CHECK_STATE_HEADER, 
    CHECK_STATE_CONTENT 
};

enum LINE_STATUS {
    LINE_OK = 0, 
    LINE_BAD, 
    LINE_KPON 
};

enum HTTP_CODE {
    REQUEST_KPON, 
    REQUEST_GET, 
    REQUEST_POST,
    REQUEST_BAD, 
    REQUEST_END,             //读到尽头
    REQUEST_NET_ERROR,       //internet error  
    REQUEST_CLOSED_CONN      //closed connection
};

#endif
