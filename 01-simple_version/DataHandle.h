#ifndef _DATA_HANDLE_H_
#define _DATA_HANDLE_H_
#include <boost/noncopyable.hpp>
#include "csapp.h"

class DataHandle : boost::noncopyable
{
public:
	DataHandle(int commufd)
		: sockfd_(-1)
		, commufd_(commufd)
	{}
	~DataHandle() {
		if (sockfd_ != -1)
			utility::Close(sockfd_);
	}
public:
	void Handle();
private:
	void Accept();
	void PasvListen();
	void PosiListen();
private:
	int sockfd_; /* 这个用于和客户端数据的交互 */
	int commufd_; /* 一条管道用于进程间的交流 */
};

#endif

