#ifndef _DATA_HANDLE_H_
#define _DATA_HANDLE_H_
#include <boost/noncopyable.hpp>
#include <string>
#include "csapp.h"

class DataHandle : boost::noncopyable
{
public:
	DataHandle(int commufd)
		: sockfd_(-1)
		, commufd_(commufd)
		, passive_(true)
	{}
	~DataHandle() {
		if (sockfd_ != -1)
			utility::Close(sockfd_);
		utility::Close(commufd_);
		//printf("~DataHandle\n");
	}
public:
	void Handle();
private:
	void GotFd();
	void PasvListen();
	void SetPeerAddr();
	const char* GetLocalIP();
private:
	int sockfd_; /* 这个用于和客户端数据的交互 */
	int commufd_; /* 一条管道用于进程间的交流 */
	bool passive_;
	std::string ip_; /* ip地址 */
	uint16_t port_; /* 端口 */
};

#endif

