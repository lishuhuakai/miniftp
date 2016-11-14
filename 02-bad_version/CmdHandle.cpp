#include <string.h>
#include <stdarg.h>
#include <boost/shared_ptr.hpp>
#include <utility>
#include <sys/types.h>
#include <dirent.h>
#include <sys/sendfile.h>
#include "csapp.h"
#include "CmdHandle.h"
#include "ftp_codes.h"
#include "wrapper.h"
#include "Cmd.h"

using namespace utility;

/*-
* 想象很好,但是实际却很不理想,一个连接用来控制命令,一个连接用来控制数据连接,想法很好,实际操作起来有很大的困难.
*/

CmdHandle::CmdHandle(int cmdfd, int commufd)
	: cmdfd_(cmdfd)
	, commufd_(commufd)
{ 
	router_["FEAT"] = boost::bind(&CmdHandle::FEAT, this);
	router_["SYST"] = boost::bind(&CmdHandle::SYST, this);
	router_["PWD"] = boost::bind(&CmdHandle::PWD, this);
	router_["OPTS"] = boost::bind(&CmdHandle::OPTS, this);
	router_["CWD"] = boost::bind(&CmdHandle::CWD, this);
	router_["PASV"] = boost::bind(&CmdHandle::PASV, this);
	router_["PORT"] = boost::bind(&CmdHandle::PORT, this);
	router_["LIST"] = boost::bind(&CmdHandle::LIST, this);
	router_["CDUP"] = boost::bind(&CmdHandle::CDUP, this);
	router_["RETR"] = boost::bind(&CmdHandle::RETR, this);
	router_["SIZE"] = boost::bind(&CmdHandle::SIZE, this);
	router_["TYPE"] = boost::bind(&CmdHandle::TYPE, this);
	router_["STOR"] = boost::bind(&CmdHandle::STOR, this);
	router_["MKD"] = boost::bind(&CmdHandle::MKD, this);
	router_["DELE"] = boost::bind(&CmdHandle::DELE, this);
	router_["QUIT"] = boost::bind(&CmdHandle::QUIT, this);
}


void CmdHandle::QUIT() {
	Reply("%d Goodbye.\r\n", FTP_GOODBYE);
	exit(0); /* 成功结束 */
}

void CmdHandle::Handle() { /* 全局唯一一个入口函数 */		   
	char buf[1024];
	Reply("220 Tiny Ftp Server v0.1\r\n");
	Login(); /* 首先要解决的是login的问题 */
	for (; ; ) {
		GetLine(buf, sizeof buf);
		Parsing(buf);
		boost::function<void()> &func = router_.at(cmd_);
		if (func) {
			func();
		}
		else
			Reply("%d Unknown command.\r\n", FTP_BADCMD); /* 没有找到相对应的命令处理函数 */
	}

}

void CmdHandle::SYST() { /* Syst命令,返回操作系统的类型 */
	Reply("%d Unix Type: L8\r\n", FTP_SYSTOK);
}

void CmdHandle::Reply(const char *format, ...) { /* 发送回复信息 */
	char buf[512];
	bzero(buf, sizeof buf);
	va_list v;
	va_start(v, format);
	vsnprintf(buf, sizeof buf, format, v); /* 这里不能替换成snprintf */
	va_end(v);
	Write(cmdfd_, buf, strlen(buf));
}


void CmdHandle::Login() {
	char buf[1024];

	GetLine(buf, sizeof buf);
	/* 不管读取的是什么,先回复OK */
	Reply("%d Welcome!\r\n", FTP_LOGINOK); /* 登陆成功 */
}

void CmdHandle::PWD() { /* 得到当前的目录 */
	char buf[1024] = { 0 };
	char home[1024] = { 0 };
	getcwd(home, sizeof home);
	sprintf(buf, "\"%s\"", home);
	Reply("%d %s\r\n", FTP_PWDOK, buf);
}

void CmdHandle::OPTS() {
	Reply("%d All is well!\r\n", FTP_OPTSOK); /* 好吧,直接选择OK啦! */
}

void CmdHandle::Parsing(char* cmdLine) { /* 解析命令和参数 */
	size_t len = strlen(cmdLine);
	char* start = cmdLine;
	char* end = cmdLine + len;
	char* pos = std::find(cmdLine, end, ' ');
	if (pos != end)
		cmd_.assign(cmdLine, pos); /* 得到命令行 */
	else {
		cmd_.assign(cmdLine, end - 2); /* 仅有命令,那么去掉结尾的\r\n */
		return;
	}
	start = pos + 1;
	argv_.assign(start, end - 2); /* 去掉结尾的\r\n */
}

void CmdHandle::CWD() { /* 进入到某个文件夹中 */
	CMD cmd = kCWD;
	State state;
	Write(commufd_, &cmd, sizeof(cmd)); /* 传递命令 */
	Write(commufd_, argv_.c_str(), argv_.size()); /* 传递参数 */
	chdir(argv_.c_str()); /* 改变当前目录 */
	Read(commufd_, &state, sizeof(state));
	if (Error == state)
		Reply("%d Failed to change directory!\r\n", FTP_NOPERM);
	else 
		Reply("%d Directory successfully changed!\r\n", FTP_CWDOK);
}

void CmdHandle::PASV() { /* passive模式代表客户端来连接服务器,双方来协商一些连接的信息 */
	CMD cmd = kExpectPort;
	Write(commufd_, &cmd, sizeof(cmd));
	Reply("%d Entering Entering Passive Mode (192.168.44.221.%u.%u)\r\n", FTP_PASVOK, 1025 >> 8, 1025 & 0xFF); /* 1025号端口写 */
}

void CmdHandle::LIST() { /* 列出所有的文件 */
	Reply("%d Here comes the directory listing.\r\n", FTP_DATACONN);
	/* 然后开始传输列表 */
	CMD cmd = kList;
	State state;
	Write(commufd_, &cmd, sizeof(cmd)); /* 向另外一个进程写数据 */
	bool detail = argv_.size() > 0 ? true : false; /* 是否要传递所有的细节 */
	printf("detail? %d\r\n", detail);

	Read(commufd_, &state, sizeof(state));
	if (state == DirOpenError) { /* 目录打开失败 */
		Reply("%d Directory send not OK.\r\n", FTP_NOPERM); /* 没有权限 */
	}
	else {
		Reply("%d Directory send OK.\r\n", FTP_TRANSFEROK);
	}
	
}

void CmdHandle::FEAT() { /* 处理Feat命令 */
	Reply("%d-Features:\r\n", FTP_FEAT);
	Reply(" EPRT\r\n");
	Reply(" EPSV\r\n");
	Reply(" MDTM\r\n");
	Reply(" PASV\r\n");
	Reply(" REST STREAM\r\n");
	Reply(" SIZE\r\n");
	Reply(" TVFS\r\n");
	Reply(" UTF8\r\n");
	Reply("%d End\r\n", FTP_FEAT);
}

void CmdHandle::GetLine(char* buf, size_t len) { /* 读取一行数据 */
	bzero(buf, len);
	Read(cmdfd_, buf, len);
}

void CmdHandle::PORT() { /* 来处理port命令 */
	unsigned int v[6];
	sscanf(argv_.c_str(), "%u, %u, %u, %u, %u, %u", &v[2], &v[3], &v[4], &v[5], &v[0], &v[1]); /* 主要用于获得对方的一系列信息 */
	bzero(&clnaddr_, sizeof(clnaddr_));
	clnaddr_.sin_family = AF_INET;
	unsigned char* p = reinterpret_cast<unsigned char *>(&clnaddr_.sin_port);
	/* 网络字节序 */
	p[0] = v[0];
	p[1] = v[1];

	p = (unsigned char*)&(clnaddr_.sin_addr);
	p[0] = v[2];
	p[1] = v[3];
	p[2] = v[4];
	p[3] = v[5];
	Reply("%d PORT command successful. Consider using PASV.\r\n", FTP_PORTOK);
}

void CmdHandle::CDUP() { /* 进入上一个文件夹 */
	CMD cmd = kCDUP;
	State state;
	Write(commufd_, &cmd, sizeof(cmd));
	chdir(".."); /* 好吧,用两个进程来同步干一些事情确实挺傻的! */
	Read(commufd_, &state, sizeof(state));
	if (state == Error) {
		Reply("%d Failed to change directory.\r\n", FTP_NOPERM);
		return;
	}
	Reply("%d Directory successfully changed.\r\n", FTP_CWDOK);
}

void CmdHandle::SIZE() { /* 获取文件的大小 */
	struct stat fileInfo;
	if (stat(argv_.c_str(), &fileInfo) < 0) {
		Reply("%d SIZE operation fail.\r\n", FTP_FILEFAIL);
		return;
	}

	if (!S_ISREG(fileInfo.st_mode)) { /* 没有权限获得文件的信息 */
		Reply("%d Could not get file size.\r\n", FTP_FILEFAIL);
		return;
	}
	char buf[20] = { 0 };
	sprintf(buf, "%lld", (long long)fileInfo.st_size);
	Reply("%d %s\r\n", FTP_SIZEOK, buf);
}

void CmdHandle::RETR() {
	Connection conn;
	int fd = open(argv_.c_str(), O_RDONLY, NULL); /* 打开文件 */
	if (fd < 0) {
		Reply("%d Open local file fail.\r\n", FTP_FILEFAIL);
		return;
	}

	/* 开始传送文件 */
	char text[1024] = { 0 };
	struct stat fileInfo;
	fstat(fd, &fileInfo);

	if (!S_ISREG(fileInfo.st_mode)) { /* 判断是否为普通文件,设备文件不能下载 */
		Reply("%d It is not a regular file.\r\n", FTP_FILEFAIL);
		return;
	}

	if (mode_ != ascii) {
		sprintf(text, "Opening BINARY mode data connection for %s (%lld bytes).",
			argv_.c_str(), (long long)fileInfo.st_size);
	}
	else {
		sprintf(text, "Opening ASCII mode data connection for %s (%lld bytes).",
			argv_.c_str(), (long long)fileInfo.st_size);
	}

	Reply("%d %s\r\n", FTP_DATACONN, text);

	/*
	 *	读取文件内容,写入套接字
	 */
	
	size_t offset = 0; /* 这里是假设没有断点续传 */
	if (offset != 0) {
		Lseek(fd, offset, SEEK_SET);
	}
	size_t size = fileInfo.st_size;
	if (offset != 0) {
		size -= offset; /* 需要传送的字节的数目 */
	}

	while (size > 0) {
		int sended = sendfile(conn->GetFd(), fd, NULL, size);
		size -= sended; 
	}
	Close(fd); /* 关闭文件 */
	Reply("%d Transfer complete.\r\n", FTP_TRANSFEROK); 
}

void CmdHandle::TYPE() {
	if (argv_ == "A") {
		mode_ = ascii;
		Reply("%d Switching to ASCII mode.\r\n", FTP_TYPEOK);
	}
	else if (argv_ == "I") {
		mode_ = binary;
		Reply("%d Switching to BINARY mode.\r\n", FTP_TYPEOK);
	}
	else {
		Reply("%d Unrecognised TYPE command.\r\n", FTP_BADCMD);
	}
}

void CmdHandle::STOR() {
	Connection conn ;
	size_t offset = 0; /* 文件重新开始的地方 */
	/* store的话代表不用append */
	int fd = Open(argv_.c_str(), O_CREAT | O_WRONLY, 0666);

	if (offset == 0) {
		ftruncate(fd, 0); /* 清空文件内容 */
		Lseek(fd, 0, SEEK_SET); /* 定位到文件的开头 */
	}
	else {
		Lseek(fd, offset, SEEK_SET); /* 定位到文件的偏移位置 */
	}

	Reply("%d OK to send data.\r\n", FTP_DATACONN);

	/* 读取数据,写入本地文件 */
	char buf[2048];
	State state;
	for (; ; ) {
		int res = read(conn->GetFd(), buf, sizeof(buf));
		if (res == -1) {
			if (errno == EINTR)
				continue;
			else {
				state = readerror;
				break;
			}
				
		}
		else if (res == 0) { /* 对方关闭了连接 */
			break;
		}
		if (write(fd, buf, res) != res) { /* 往文件中写入数据 */
			state = writeerror;
			break;
		}
	}
	Close(fd); /* 关闭文件 */

	if (state == readerror) { /* 读出错 */
		Reply("%d Fail to read from network stream.\r\n", FTP_BADSENDNET);
	} 
	else if (state == writeerror) {
		Reply("%d Fail to write to local file.\r\n", FTP_BADSENDFILE);
	}
	else {
		Reply("%d Transfer complete.\r\n", FTP_TRANSFEROK); /* 文件传送成功 */
	}
	
}


void CmdHandle::DELE() {
	if (unlink(argv_.c_str()) < 0) {
		Reply("%d Delete operation fail.\r\n", FTP_NOPERM);
		return;
	}
	Reply("%d Delete operation successful.\r\n", FTP_DELEOK);
}

void CmdHandle::MKD() {
	if (mkdir(argv_.c_str(), 0777) < 0) {
		Reply("%d Create directory operation fail.\r\n", FTP_FILEFAIL);
		return;
	}
	Reply("%d Directory successfully created!\r\n", FTP_MKDIROK);
}