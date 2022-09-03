#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include "./Server/server.h"
#include "./Log/log.h"

void get_path(char* path, int size);	//获取资源文件路径

int main(int argc, char *argv[]){	
	int sql_port = 3306;
	string sql_url = "127.0.0.1";
	string sql_user = "root";
	string sql_passwd = "068319abc";
	string sql_database = "User";
	
	char path[PATH_SIZE] = {0};	//strcat:需要清空char[]
	get_path(path, PATH_SIZE);
	
	assert(argc == 2);
	int port = atoi(argv[1]);
	pid_t pid = fork();
	assert(pid >= 0);

	if (pid > 0) {
		//需要使用kill+pid才能终止进程
		std::cout<<"守护进程启动!"<<std::endl;  
		std::cout<<"进程id = "<<pid<<std::endl;
	}
	else if (pid == 0) 
	{
		LOG("info","");
		LOG("info","");
		LOG("info","");
		LOG("info","");
		LOG("info","My Server start!");
		Server myServer;
		myServer.init(port, path, sql_port, sql_url, sql_user, sql_database, sql_passwd);
		myServer.event_loop();
	}
}


void get_path(char* path, int size) {
    getcwd(path, size);
    char tar[7] = "/Src/"; //只需修改此处即可修改资源文件的路径
    strcat(path, tar);
}