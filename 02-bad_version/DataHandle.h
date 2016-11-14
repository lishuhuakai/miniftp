#ifndef _DATA_HANDLE_H_
#define _DATA_HANDLE_H_
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "csapp.h"
#include "Cmd.h"


class Conn;
class DataHandle : boost::noncopyable
{
public:
	typedef boost::shared_ptr<Conn> Connection;
	DataHandle(int commufd)
		: sockfd_(-1)
		, commufd_(commufd)
		, passive_(true)
	{}
	~DataHandle() {
		if (sockfd_ != -1)
			utility::Close(sockfd_);
	}
public:
	void Handle();
private:
	void PasvListen();
	void SendList(bool detail);
	void PosiListen();
	void ChangeDir(const char* dir);
	Connection GetConnect();
private:
	int sockfd_; /* 这个用于和客户端数据的交互 */
	int commufd_; /* 一条管道用于进程间的交流 */
	bool passive_; /* 是否为被动模式 */
	utility::State state_;
};

#endif

