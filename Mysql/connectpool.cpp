#include "./connectpool.h"


Connectpool::Connectpool() {
	this->cur_conn_num = 0;
	this->free_conn_num = 0;
    this->max_conn_num = MAX_CONN_NUM;
}


Connectpool::~Connectpool() {
	destroy_pool();
}


Connectpool* Connectpool::get_instance() {
	static Connectpool conn_pool;
	return &conn_pool;
}


//构造初始化
void Connectpool::init( int port, string url, string user, string passwd, string database)
{
	this->url = url;
	this->port = port;
	this->user = user;
	this->passwd = passwd;
	this->database = database;

	mutexlock.lock();
	for (int i = 0; i < max_conn_num; ++i) 
    {
		MYSQL *con = NULL;
		con = mysql_init(con);
		if (con == NULL) {
			perr("connectpool::Init connection");
		}
		con = mysql_real_connect(con, url.c_str(), user.c_str(), passwd.c_str(), database.c_str(), port, NULL, 0);
		if (con == NULL) {
			perr("connectpool::create connection");
		}
		pool.push_back(con);
		++free_conn_num;
	}
    sem = Sem(free_conn_num);
	mutexlock.unlock();
}


//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *Connectpool::get_conn() {
	MYSQL *con = NULL;

	if (0 == pool.size())
		return NULL;

	sem.wait();
	mutexlock.lock();

	con = pool.front();
	pool.pop_front();

	--free_conn_num;
	++cur_conn_num;

	mutexlock.unlock();
	LOG("info","connectpool::get_conn::get one connect...");
	return con;
}


//释放当前使用的连接
bool Connectpool::release_conn(MYSQL *con) {
	if (NULL == con) 
        return false;

	mutexlock.lock();
	
    pool.push_back(con);
	++free_conn_num;
	--cur_conn_num;
	
    mutexlock.unlock();
	sem.post();
	return true;
}


//销毁数据库连接池
void Connectpool::destroy_pool()
{
	mutexlock.lock();
	if (pool.size() > 0) {
		list<MYSQL *>::iterator it;
		for (auto each : pool) {
			mysql_close(each);
		}
		cur_conn_num = 0;
		free_conn_num = 0;
		pool.clear();

		mutexlock.unlock();
	}
	mutexlock.unlock();
}


//当前空闲的连接数
int Connectpool::get_free_conn_num() {
	return this->free_conn_num;
}

