#include "DataHandle.h"
#include <string.h>
#include "csapp.h"
#include "Cmd.h"
#include "wrapper.h"
using namespace utility;


void DataHandle::Handle() {
	for (; ; ) { /* 暂时什么事情也不干 */
		CMD cmd;
		readn(commufd_, &cmd, sizeof(cmd));
		switch (cmd) {
		case kExpectPort:
			printf("recv kExpectPort!\n");
			PasvListen(); /* 被动监听 */
			break;
		case kExpectFd:
			printf("recv kExpectFd!\n");
			Accept();
			break;
		case kExpectConn:
			printf("recv kExpectConn!\n"); /* 期待值连接的到来 */
			PosiListen(); /* 主动监听 */
		default:
			break;
		}
	}
}

void DataHandle::PasvListen() {
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

void DataHandle::Accept() {
	int fd = ::Accept(sockfd_, NULL, NULL); /* 获得一个连接 */
	CMD cmd = kGotFd;
	Write(commufd_, &cmd, sizeof(cmd)); /* 先发送成功的消息 */
	sendfd(commufd_, fd); /* 发送文件描述符 */
	close(fd); /* 我有一个疑问,那就是这个套接字应该不共享吧! */
}
