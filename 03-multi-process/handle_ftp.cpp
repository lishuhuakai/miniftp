#include <boost/bind.hpp>
#include <stdarg.h>
#include <utility>

#include "handle_ftp.h"
#include "csapp.h"
#include "wrapper.h"
#include "CmdHandle.h"
#include "DataHandle.h"
using namespace utility;

HandleFtp::HandleFtp(int cmdfd)
	: cmdfd_(cmdfd) /* cmdfd表示连接的玩意 */
{ }

void HandleFtp::Handle() {
	/* 首先要调用socketpair函数构建管道 */
	Socketpair(AF_UNIX, SOCK_STREAM, 0, commufd_);
	/* 然后调用Fork函数 */
	if (Fork() == 0) { /* 子进程 */
		Close(cmdfd_);
		Close(commufd_[0]); /* 关闭掉另外一端 */
		DataHandle data(commufd_[1]);
		data.Handle();
	}
	else { /* 父进程 */
		Close(commufd_[1]); /* 关闭掉一端 */
		CmdHandle cmd(cmdfd_, commufd_[0]); /* cmdfd_ */
		cmd.Handle(); /* 处理连接 */
	}
}


