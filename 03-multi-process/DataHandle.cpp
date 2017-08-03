#include "DataHandle.h"
#include <string.h>
#include "csapp.h"
#include "Cmd.h"
#include "wrapper.h"
using namespace utility;


void DataHandle::Handle() {
	for ( ; ; ) { /* 暂时什么事情也不干 */
		CMD cmd;
		int num = Read(commufd_, &cmd, sizeof(cmd));
		//printf("hi, num = %d\n", num);
		if (num == 0) { /* 对方已经关闭了连接 */
			break;
		}
		switch (cmd) {
		case kExpectPort:
			//printf("recv kExpectPort!\n");
			PasvListen(); /* 被动监听 */
			break;
		case kExpectFd:
			//printf("recv kExpectFd!\n");
			GotFd();
			break;
		case kPort:
			//printf("recv kPort!\n"); /* 接收到port命令 */
			SetPeerAddr();
			break;
		default:
			break;
		}
	}
}

void DataHandle::SetPeerAddr() { /* 设置对方的地址 */
	char ipAddr[20] = { 0 };
	Read(commufd_, &port_, sizeof(port_)); /* 获得端口 */
	//printf("port is %d\n", port_);
	Read(commufd_, &ipAddr, sizeof(ipAddr)); /* 获得ip地址 */
	//printf("ip address is %s\n", ipAddr);
	ip_.assign(ipAddr, ipAddr + strlen(ipAddr));
	passive_ = false;
}

void DataHandle::PasvListen() {
	if (sockfd_ == -1) { /* 已经初始化过了,就不再初始化了 */
		/* 数据处理这端开始监听 */
		sockfd_ = Open_listenfd(0); /* 0代表任意选择一个端口 */
		struct sockaddr_in localaddr;
		socklen_t len = sizeof(localaddr);
		Getsockname(sockfd_, (SA*)&localaddr, &len);
		port_ = ntohs(localaddr.sin_port); /* 得到端口号 */
		GetLocalIP();
	}
	int size = ip_.size();
	Write(commufd_, &size, sizeof(size)); /* 先传递长度 */
	Write(commufd_, ip_.c_str(), size); /* 传递ip地址 */
	Write(commufd_, &port_, sizeof(port_)); /* 传递端口值 */
	//printf("Hi, I am listening!\n");
}

void DataHandle::GotFd() {
	int fd;
	if (passive_) { /* 如果是被动模式的话,就直接获得对方的一个连接就行了 */
		fd = Accept(sockfd_, NULL, NULL); /* 获得一个连接 */
	} 
	else { /* 主动模式有点问题,主要是在局域网里连接不到 */
		/* 下面的Open_clientfd函数用于连接客户端,里面有调用connnet函数 */
		fd = Open_clientfd(ip_.c_str(), port_); /* 端口填0的话,会随机分配一个端口 */
	}
	//printf("ip : %s\n", ip_.c_str());
	State state = Success;
	Write(commufd_, &state, sizeof(state)); /* 先发送成功的消息 */
	//printf("Got fd! %d\n", fd);
	sendfd(commufd_, fd); /* 发送文件描述符,需要注意的是,文件描述符在两个不同的进程间传递 */
	close(fd);  /* 传输完文件描述符后一定要记得关闭这个文件描述符 */
}

const char* DataHandle::GetLocalIP() { /* 得到本机的ip地址 */
	if (ip_.size() != 0) return ip_.c_str(); 
	char ip[20] = { 0 };
	Getlocalip(ip);
	ip_ = ip;
	return ip_.c_str();
}
