#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include "csapp.h"
#include "handle_ftp.h"
using namespace utility;


int main(int argc, char *argv[])
{
	int listenfd = Open_listenfd(1024); /* 监听套接字 */
	struct sockaddr_in clnaddr;
	socklen_t len = sizeof(clnaddr);

	for ( ; ; ) {
		int connfd = Accept(listenfd, (SA*)&clnaddr, &len);
		if (Fork() == 0) { /* 如果是子进程的话 */
			Close(listenfd);
			HandleFtp handle(connfd);
			handle.Handle();
			break;
		}
		else {
			Close(connfd);
		}
	}
	return 0;
}