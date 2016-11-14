#ifndef _DATA_CONNECT_H_
#define _DATA_CONNECT_H_
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "csapp.h"

class Conn : boost::noncopyable
{
public:
	Conn(int fd)
		: sockfd_(fd)
	{}
	~Conn() {
		printf("Close connection!\n");
		utility::Close(sockfd_); /* 析构的时候关闭连接 */
	}
	int GetFd() {
		return sockfd_;
	}

private:
	int sockfd_;
};

#endif
