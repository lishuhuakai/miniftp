#include "wrapper.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <unistd.h>
#include <sys/ioctl.h>

namespace utility {
	extern void unix_error(const char* msg);
	extern void app_error(const char *msg);

	int Socketpair(int domain, int type, int protocol, int fd[2]) {
		int res;
		if ((res = socketpair(domain, type, protocol, fd)) < 0) {
			unix_error("socketpair error");
		}
		return res;
	}

	ssize_t readn(int fd, void *vptr, size_t n) { /* 从描述符中读取n个字符 */
		size_t nleft; /* 剩余的字节数目 */
		ssize_t nread; /* 已经读取了的字节数目 */
		char *ptr;

		ptr = (char *)vptr;
		nleft = n;
		while (nleft > 0) {
			if ((nread = read(fd, ptr, nleft)) < 0) {
				if (errno == EINTR)  /* 被其他信号中断 */
					nread = 0;
				else
					unix_error("readn error");
			}
			else if (nread == 0)
				break;
			nleft -= nread;
			ptr += nread;
		}
		return (n - nleft);
	}

	ssize_t written(int fd, const void *vptr, size_t n) {
		size_t nleft;
		ssize_t nwritten; /* 已经写了的字节数目 */
		const char *ptr;

		ptr = (const char*)vptr;
		nleft = n;
		while (nleft > 0) {
			if ((nwritten = write(fd, ptr, nleft)) <= 0) {
				if (nwritten < 0 && errno == EINTR)
					nwritten = 0;
				else
					unix_error("writen error");
			}
			nleft -= nwritten;
			ptr += nwritten;
		}
		return n; /* 返回写入的字节数目 */
	}

	int Fcntl(int fd, int cmd, void *arg) {
		int n;
		if ((n = fcntl(fd, cmd, arg)) == -1)
			unix_error("fcntl error");
		return (n);
	}

	const char* getperms(struct stat& sbuf) { /* 获得文件的权限 */
		static char perms[] = "----------";
		perms[0] = '?';

		mode_t mode = sbuf.st_mode;
		switch (mode & S_IFMT) {
		case S_IFREG:
			perms[0] = '-';
			break;
		case S_IFLNK:
			perms[0] = 'l'; /* 链接文件 */
			break;
		case S_IFDIR:
			perms[0] = 'd'; /* 目录文件 */
			break;
		case S_IFIFO:
			perms[0] = 'p'; /* 管道文件 */
			break;
		case S_IFCHR:
			perms[0] = 'c';
			break;
		case S_IFSOCK:
			perms[0] = 's'; /* socket文件 */
			break;
		case S_IFBLK:
			perms[0] = 'b'; /* 块文件吗? */
		}

		if (mode & S_IRUSR) { /* 获得权限值 */
			perms[1] = 'r';
		}

		if (mode & S_IWUSR) {
			perms[2] = 'w';
		}

		if (mode & S_IXUSR) {
			perms[3] = 'r';
		}

		if (mode & S_IWGRP) {
			perms[4] = 'r';
		}

		if (mode & S_IWGRP) {
			perms[5] = 'w';
		}

		if (mode & S_IXGRP) {
			perms[6] = 'x';
		}

		if (mode & S_IROTH) {
			perms[7] = 'r';
		}

		if (mode & S_IWOTH) {
			perms[8] = 'w';
		}

		if (mode & S_IXOTH) {
			perms[9] = 'x';
		}

		if (mode & S_ISUID) {
			perms[3] = (perms[3] == 'x') ? 's' : 'S';
		}

		if (mode & S_ISGID) {
			perms[6] = (perms[6] == 'x') ? 's' : 'S';
		}

		if (mode & S_ISVTX) {
			perms[9] = (perms[9] == 'x') ? 't' : 'T';
		}

		return perms;
	}

	const char* getdate(struct stat& sbuf) {
		static char datebuf[64] = { 0 };
		const char* format = "%b %e %H:%M";
		struct timeval tv;
		gettimeofday(&tv, NULL);
		time_t localTime = tv.tv_sec;
		if (sbuf.st_mtime > localTime || (localTime - sbuf.st_mtime) > 60*60*24*182) {
			format = "%b %e %Y";
		}
		struct tm* p = localtime(&sbuf.st_mtime); /* 这些玩意处理起来还真是麻烦! */
		strftime(datebuf, sizeof(datebuf), format, p); /* 将格式处理成字符串 */
		return datebuf;
	}

	void sendfd(int sockfd, int fd) { /* 在进程间传递文件描述符 */
		int ret;
		struct msghdr msg;
		struct cmsghdr *p_cmsg;
		struct iovec vec;
		char cmsgbuf[CMSG_SPACE(sizeof(fd))];
		int *p_fds;
		char sendchar = 0;
		msg.msg_control = cmsgbuf;
		msg.msg_controllen = sizeof(cmsgbuf);
		p_cmsg = CMSG_FIRSTHDR(&msg);
		p_cmsg->cmsg_level = SOL_SOCKET;
		p_cmsg->cmsg_type = SCM_RIGHTS;
		p_cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
		p_fds = (int*)CMSG_DATA(p_cmsg);
		*p_fds = fd;

		msg.msg_name = NULL;
		msg.msg_namelen = 0;
		msg.msg_iov = &vec;
		msg.msg_iovlen = 1;
		msg.msg_flags = 0;

		vec.iov_base = &sendchar;
		vec.iov_len = sizeof(sendchar);
		ret = sendmsg(sockfd, &msg, 0);
		if (ret != 1)
			unix_error("sendmsg");
	}

	int recvfd(int sockfd) { /* 接收文件描述符 */
		int ret;
		struct msghdr msg;
		char recvchar;
		struct iovec vec;
		int recv_fd;
		char cmsgbuf[CMSG_SPACE(sizeof(recv_fd))];
		struct cmsghdr *p_cmsg;
		int *p_fd;
		vec.iov_base = &recvchar;
		vec.iov_len = sizeof(recvchar);
		msg.msg_name = NULL;
		msg.msg_namelen = 0;
		msg.msg_iov = &vec;
		msg.msg_iovlen = 1;
		msg.msg_control = cmsgbuf;
		msg.msg_controllen = sizeof(cmsgbuf);
		msg.msg_flags = 0;

		p_fd = (int*)CMSG_DATA(CMSG_FIRSTHDR(&msg));
		*p_fd = -1;
		ret = recvmsg(sockfd, &msg, 0);
		if (ret != 1)
			unix_error("recvmsg");

		p_cmsg = CMSG_FIRSTHDR(&msg);
		if (p_cmsg == NULL)
			unix_error("no passed fd");


		p_fd = (int*)CMSG_DATA(p_cmsg);
		recv_fd = *p_fd;
		if (recv_fd == -1)
			unix_error("no passed fd");

		return recv_fd;
	}

	int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len) { /* 文件锁 */
		struct flock lock;
		lock.l_type = type;
		lock.l_start = offset;
		lock.l_whence = whence;
		lock.l_len = len;
		return (fcntl(fd, cmd, &lock));
	}

	long long str2longlong(const char* str) { /* 将字符串转换成为long long类型 */
		long long res = 0;
		size_t len = strlen(str);
		int multi = 1;
		for (int i = len - 1; i >= 0; --i) {
			char ch = str[i];
			if (ch < '0' || ch > '9')
				return 0;
			res += (ch - '0') * multi;
			multi *= 10;
		}
		return res;
	}


	void Getlocalip(char* ip) {
		int res;
		if ((res = getlocalip(ip)) == -1) {
			unix_error("Getlocakip error");
		}
	}

	int getlocalip(char * ip) {
		int fd;
		struct sockaddr_in* addr;
		struct ifreq ifr;

		if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			return -1;
		}

		strcpy(ifr.ifr_ifrn.ifrn_name, "eth0"); /* 接口的名称 */

		if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) { /* 通过内核获取接口的信息 */
			return -1; /* ioctl以及socket函数出错都会设置errno */
		}

		addr = (sockaddr_in *)&ifr.ifr_ifru.ifru_addr; /* 获得ip地址 */
		strcpy(ip, inet_ntoa(addr->sin_addr));
		close(fd);
		return 0; /* 成功 */
	}
}