#ifndef _HANDLE_FTP_H_
#define _HANDLE_FTP_H_
#include <boost/noncopyable.hpp>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

class HandleFtp : boost::noncopyable
{
public:
	HandleFtp(int cmdfd);
	void Handle();
private:
	int cmdfd_; /* 这个文件描述符用于传递命令 */
	int commufd_[2]; /* 这个管道用于父子进程通信 */
};

#endif
