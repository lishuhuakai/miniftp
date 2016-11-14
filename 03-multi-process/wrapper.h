#ifndef _WRAPPER_H_
#define _WRAPPER_H_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace utility {
	int Socketpair(int, int, int, int fd[2]);
	ssize_t written(int fd, const void *vptr, size_t n);
	ssize_t readn(int fd, void *vptr, size_t n);
	const char* getperms(struct stat& sbuf);
	const char* getdate(struct stat& sbuf); /* 得到时间的信息 */
	void sendfd(int sockfd, int fd); /* 发送文件描述符 */
	int recvfd(int sockfd); /* 接收文件描述符 */

	int Fcntl(int fd, int cmd, void *arg);

	long long str2longlong(const char* str);

	/* 文件锁 */
	int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len);

#define read_lock(fd, offset, whence, len) \
	utility::lock_reg(fd, F_SETLK, F_RDLCK, offset, whence, len)

#define readw_lock(fd, offset, whence, len) \
	utility::lock_reg(fd, F_SETLKW, F_RDLCK, offset, whence, len)

#define write_lock(fd, offset, whence, len) \
	utility::lock_reg(fd, F_SETLK, F_WRLCK, offset, whence, len)

#define writew_lock(fd, offset, whence, len) \
	utility::lock_reg(fd, F_SETLKW, F_WRLCK, offset, whence, len)

#define un_lock(fd, offset, whence, len) \
	utility::lock_reg(fd, F_SETLK, F_UNLCK, offset, whence, len)

	void Getlocalip(char* ip);

	int getlocalip(char * ip);
}

#endif
