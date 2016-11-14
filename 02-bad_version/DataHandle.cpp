#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/sendfile.h>
#include "DataHandle.h"
#include "csapp.h"
#include "wrapper.h"
#include "Connect.h"
#include "Cmd.h"
using namespace utility;


void DataHandle::Handle() {
	for ( ; ; ) { /* 暂时什么事情也不干 */
		CMD cmd;
		readn(commufd_, &cmd, sizeof(cmd));
		switch (cmd) {
		case kExpectPort:
			printf("recv kExpectPort!\n");
			PasvListen(); /* 被动监听 */
			break;
		case kExpectConn:
			printf("recv kExpectConn!\n"); /* 期待值连接的到来 */
			PosiListen(); /* 主动监听 */
			break;
		case kList:
			printf("recv kList!\n");
			SendList(true);
		case kCDUP:
			printf("recv kCDUP!\n");
			ChangeDir("..");
			break;
		case kCWD: {
			printf("recv kCWD!\n");
			char buf[1024] = { 0 };
			Read(commufd_, buf, sizeof(buf)); /* 读入文件夹的地址 */
			printf("dir is %s\n", buf);
			ChangeDir(buf);
		}

		default:
			break;
		}
	}
}

void DataHandle::ChangeDir(const char* dir) { /* 两个进程间进行通信究竟是不是好事,我就不知道了! */
	if (chdir(dir) < 0) {
		state_ = Error;
		Write(commufd_, &state_, sizeof(state_));
	}
	else {
		state_ = Success;
		Write(commufd_, &state_, sizeof(state_));
	}
}

void DataHandle::PasvListen() { /* 创建套接字 */
	if (sockfd_ == -1) {
		sockfd_ = Open_listenfd(1025); /* 在1025号端口上监听 */
		printf("Hi, I am listening!\n");
	}
		
}

void DataHandle::PosiListen() {
	unsigned short port;
	Read(commufd_, &port, sizeof(port)); /* 首先读取端口号 */
	printf("Got port! %d \n", port);
	char ip[20] = { 0 };
	size_t len;
	Read(commufd_, &len, sizeof(len)); /* 读取ip地址的长度 */
	printf("len is %d\n", len);
	Read(commufd_, ip, len); /* 然后读取对方的ip地址 */
	printf("Got ip! %s \n", ip);

	int fd = Open_clientfd(ip, port);
	printf("Got fd! %d \n", fd);
	CMD cmd = kGotFd;
	Write(commufd_, &cmd, sizeof(cmd));
	sendfd(commufd_, fd); /* 发送连接 */
}


void DataHandle::SendList(bool detail = true) {
	Connection conn = GetConnect(); /* 得到连接 */

	DIR* dir = opendir(".");
	if (dir == NULL) { /* 目录打开失败 */
		state_ = DirOpenError;
		Write(commufd_, &state_, sizeof(state_));
		return;
	}

	struct dirent* ent;
	struct stat sbuf;

	while ((ent = readdir(dir)) != NULL) { /* 读取文件夹的信息 */
		if (lstat(ent->d_name, &sbuf) < 0)
			continue;
		if (strncmp(ent->d_name, ".", 1) == 0)
			continue; /* 忽略隐藏的文件 */
		const char* permInfo = getperms(sbuf); /* 获得权限值 */
		char buf[1024] = { 0 };

		if (detail) { /* 如果要发送完整的信息的话 */
			int offset = 0;
			offset += sprintf(buf, "%s ", permInfo);
			offset += sprintf(buf + offset, " %3d %-8d %-8d ", sbuf.st_nlink, sbuf.st_uid, sbuf.st_gid);
			offset += sprintf(buf + offset, "%8lu ", (unsigned long)sbuf.st_size); /* 文件大小 */
			const char *dateInfo = getdate(sbuf); /* 获得时间的信息 */
			offset += sprintf(buf + offset, "%s ", dateInfo);
			if (S_ISLNK(sbuf.st_mode)) { /* 如果表示的是链接文件的话 */
				char name[1024] = { 0 };
				readlink(ent->d_name, name, sizeof(name));
				offset += sprintf(buf + offset, "%s -> %s\r\n", ent->d_name, name);
			}
			else {
				offset += sprintf(buf + offset, "%s\r\n", ent->d_name);
			}
		}
		else {
			sprintf(buf, "%s\r\n", ent->d_name);
		}
		Write(conn->GetFd(), buf, strlen(buf));
	}
	closedir(dir); /* 关闭文件夹 */
	state_ = SendListOk;
	Write(commufd_, &state_, sizeof(state_));
}

DataHandle::Connection DataHandle::GetConnect() { /* 获得连接 */
	if (passive_) { /* 如果是被动连接模式 */
		int fd = ::Accept(sockfd_, NULL, NULL); /* 获得一个连接 */
		return boost::make_shared<Conn>(fd);
	}
	else { /* 如果是主动连接模式 */

	}
}
