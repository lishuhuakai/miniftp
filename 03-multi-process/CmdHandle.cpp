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
#include "fileLocker.h"
using namespace utility;


SignalHandle g_sighandle; /* 全局一个信号处理器 */

void CmdHandle::HandleUrg() { /* 处理紧急数据 */
	/*
	 * 一般而言,对于正在传输中的数据连接,客户端会给服务器发送紧急数据,当然,我这里也没有处理,直接丢弃.
	 * 发送的紧急数据是什么呢? \377\364 \377\362之类的.
	 * \377\364 \377\362就是telnet协议中规定的操作，我翻译一下(操作序列)：'IAC' 'Interrupt Process' 'IAC' 'Data Mark'
     * 其中：(１) \364表示操作：'Interrupt Process'，即实施telnet的The function IP。含义：ftp客户机告诉你这个FTP服务器，
	 * 赶快放下你现在手头的事情，马上处理我的事件(我有紧急数据到来)。
	 * (２) \362表示操作：'Data Mark',这个字节是ftp客户机以TCP的紧急模式发送的一个字节。含义：即：其后的数据必须立即读取。
	 * (３) \377即IAC，是telnet中的转义字节（即：255），每一个telnet操作（如：\364、\362）都必须以IAC开始。
	 * 如何处理? 因为f2是通过TCP紧急模式发送的一个字节而已。你只要将字节f2（即telnet操作：\362）丢弃即可以了。以上仅供你参考.
	 */ 
	char cmd[256] = { 0 };
	int errorno = 0;
	buffer_.readFd(cmdfd_, &errorno);
	strncpy(cmd, buffer_.peek(), buffer_.readableBytes()); /* 调试的时候你可以查看收到了什么东西 */
	if (errorno == 0 && false == buffer_.getLine(cmd, sizeof(cmd))) /* 没有出错 */
		buffer_.retrieveAll(); /* 丢弃掉紧急数据 */
	printf("recv urg!\n");
}

CmdHandle::CmdHandle(int cmdfd, int commufd)
	: cmdfd_(cmdfd)
	, commufd_(commufd)
	, stop_(false)
	, resumePoint_(0)
	, sighanler_(g_sighandle)
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
	router_["REST"] = boost::bind(&CmdHandle::REST, this);
	router_["ABOR"] = boost::bind(&CmdHandle::ABOR, this);
	router_["NOOP"] = boost::bind(&CmdHandle::NOOP, this);
	router_["RMD"] = boost::bind(&CmdHandle::RMD, this);
	router_["RNFR"] = boost::bind(&CmdHandle::RNFR, this);
	router_["RNTO"] = boost::bind(&CmdHandle::RNTO, this);
	router_["SITE"] = boost::bind(&CmdHandle::SITE, this);
}

void CmdHandle::RMD() {
	if (rmdir(argv_.c_str()) < 0)
		Reply("%d remove a directory failed.\r\n", FTP_NOPERM); /* 因为权限不够而失败 */
	else
		Reply("%d remove successfully.\r\n", FTP_RMDIROK);
}

void CmdHandle::RNFR() {
	/* 这个命令指定了需要重新命名的原始路径名,后面必须马上接着RNTO命令,来指定新的文件路径 */
	name_ = std::move(argv_);
	Reply("%d Ready for RNTO.\r\n", FTP_RNFROK);
}

void CmdHandle::RNTO() {
	if (name_.size() == 0) {
		Reply("%d RNFR require first.\r\n", FTP_NEEDRNFR);
		return;
	}
	rename(name_.c_str(), argv_.c_str()); /* 重命名 */
	Reply("%d Rename successful.\r\n", FTP_RENAMEOK);
	name_.clear(); /* 每次重命名完要清空name_ */
}

void CmdHandle::ABOR() {
	//Reply("%d Transfer failed.\r\n", FTP_BADSENDNET); /* 连接关闭,放弃传输 */
	/*
	 * 服务器接收这个命令时可能处在两种状态.(1) FTP服务命令已经完成了,或者(2)FTP服务还在执行中.
	 * 对于第一种情况，服务器关闭数据连接(如果数据连接是打开的)回应226代码,表示放弃命令已经成功处理.
	 * 对于第二种情况,服务器放弃正在进行的FTP服务,关闭数据连接,返回426响应代码,表示请求服务异常终止,
	 * 然后服务器发送226代码,表示放弃命令成功处理.
	 *
	 * 在我们的ABOR命令中,只发送了226代码,因为426代码在其余各个服务中发送.
	 */
	Reply("%d No transfer to Abort!\r\n", FTP_TRANSFEROK);
}

CmdHandle::~CmdHandle() {
	//printf("~CmdHandle\n");
	Close(commufd_);
	Close(cmdfd_); /* 关闭连接 */
}

void CmdHandle::QUIT() {
	Reply("%d Goodbye.\r\n", FTP_GOODBYE);
	printf("Close!");
	stop_ = true;
}

void CmdHandle::Handle() { /* 全局唯一一个入口函数 */		   
	char buf[1024];
	Reply("220 Tiny Ftp Server v0.1\r\n");
	login(); /* 首先要解决的是login的问题 */
	sighanler_.BlockSigno(SIGPIPE); /* 忽略SIGPIPE消息 */
	
	/*
	 * 这条命令还是很有必要的,因为系统可不知道cmdfd_所属的进程,所以它也不知道应该向谁发送SIGURG信号.
	 * 而设置了cmdfd_所属的进程之后,系统一旦检测到了客户端发来了紧急数据,就会立马通知该进程.否则的话
	 * 该进程是收不到SIGURG信号的.
	 */
	fcntl(cmdfd_, F_SETOWN, getpid());

	sighanler_.addSigHandle(SIGURG, boost::bind(&CmdHandle::HandleUrg, this));
	
	while (!stop_) {
		if (false == GetLine(buf, sizeof buf)) /* 等于0的话代表对方已经关闭了连接 */
			break;
		Parsing(buf);
		printf("cmd_ : %s\n", cmd_.c_str());
		//printf("argv_ : %s\n", argv_.c_str());
		boost::function<void()> &func = router_[cmd_];
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
	char buf[512] = { 0 }; /* 其实我挺讨厌使用这些固定大小的字符数组的,但是我们这个东西太小,单独写一个string又感觉太浪费 */
	va_list v;
	va_start(v, format);
	vsnprintf(buf, sizeof buf, format, v); /* 这里不能替换成snprintf */
	va_end(v);
	Write(cmdfd_, buf, strlen(buf));
}


void CmdHandle::login() {
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

void CmdHandle::Parsing(char* cmdline) { /* 解析命令和参数 */
	size_t len = strlen(cmdline);
	char* start = cmdline;
	char* end = cmdline + len;
	char* pos = std::find(cmdline, end, ' ');
	if (pos != end)
		cmd_.assign(cmdline, pos); /* 得到命令行 */
	else {
		cmd_.assign(cmdline, end); /* 仅有命令*/
		return;
	}
	start = pos + 1;
	argv_.assign(start, end);
}

void CmdHandle::CWD() {
	if (chdir(argv_.c_str()) < 0)
		Reply("%d Failed to change directory!\r\n", FTP_NOPERM);
	else 
		Reply("%d Directory successfully changed!\r\n", FTP_CWDOK);
}

void CmdHandle::PASV() { /* passive模式代表客户端来连接服务器,双方来协商一些连接的信息 */
	/* pasv模式下,服务端选择一个传输数据的端口 */
	CMD cmd = kExpectPort;
	Write(commufd_, &cmd, sizeof(cmd));
	int size;
	char ip[20] = { 0 };
	uint16_t port;
	Read(commufd_, &size, sizeof(size)); /* 得到ip地址的长度 */
	Read(commufd_, ip, size); /* 获得ip地址 */
	Read(commufd_, &port, sizeof(port)); /* 获得端口地址 */
	Reply("%d Entering Entering Passive Mode (%s.%u.%u)\r\n", FTP_PASVOK, ip, port >> 8, port & 0xFF); 
}

void CmdHandle::LIST() { /* 列出所有的文件 */
	Reply("%d Here comes the directory listing.\r\n", FTP_DATACONN);
	/* 然后开始传输列表 */
	bool detail = argv_.size() > 0 ? true : false; /* 是否要传递所有的细节 */
	//printf("detail? %d\r\n", detail);
	if (Error == Sendlist(true)) {
		Reply("%d Directory send not OK.\r\n", FTP_NOPERM); /* 没有权限 */
	}
	else {
		Reply("%d Directory send OK.\r\n", FTP_TRANSFEROK);
	}
	
}

State CmdHandle::Sendlist(bool detail) {
	Connection conn = GetConnect(); /* 得到连接 */

	DIR* dir = opendir(".");
	if (dir == NULL) /* 目录打开失败 */
		return Error;

	struct dirent* ent;
	struct stat sbuf;
	
	while ((ent = readdir(dir)) != NULL) { /* 读取文件夹的信息 */
		if (lstat(ent->d_name, &sbuf) < 0)
			continue;
		if (strncmp(ent->d_name, ".", 1) == 0)
			continue; /* 忽略隐藏的文件 */
		const char* permInfo = getperms(sbuf); /* 获得权限值 */
		char buf[1024] = { 0 };
		size_t len = sizeof(buf);

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
	return Success;
}

void CmdHandle::FEAT() { /* 处理Feat命令 */
	Reply("%d-Features:\r\n", FTP_FEAT);
	Reply(" EPRT\r\n");
	Reply(" EPSV\r\n");
	Reply(" MDTM\r\n");
	Reply(" PASV\r\n");
	Reply(" REST STREAM\r\n"); /* 断点续传 */
	Reply(" SIZE\r\n");
	Reply(" TVFS\r\n");
	Reply(" UTF8\r\n");
	Reply("%d End\r\n", FTP_FEAT);
}

bool CmdHandle::GetLine(char* buf, size_t len) { /* 读取一行数据 */
	assert(len >= 0);
	/* 读取一行数据 */
start:
	int errorno = 0;
	buffer_.readFd(cmdfd_,&errorno); /* 读取对方的数据 */
	bool res = buffer_.getLine(buf, len);
	//if (errorno == 0 && res == false) { /* errorno等于0代表没有出错 */
	//	/* 这里一般是读到了紧急数据 */
	//	buffer_.retrieveAll(); /* 清除数据 */
	//	goto start; /* 重新读取 */
	//}
	return res;
}

void CmdHandle::PORT() { /* 来处理port命令 */
	/* 关于port命令,客户端传递过来用于数据传送的ip地址和端口号,然后这个函数将其传递给另外一个进程 */
	int ip[4];
	unsigned int hPort, lPort;
	char ipAddr[20] = { 0 };
	sscanf(argv_.c_str(), "%d,%d,%d,%d,%u,%u", &ip[0], &ip[1], &ip[2], &ip[3], &hPort, &lPort); /* 主要用于获得对方的一系列信息 */
	snprintf(ipAddr, sizeof(ipAddr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]); /* 获得字符类型的ip */
	uint16_t port = (hPort << 8) | (lPort & 0xFF); /* 得到port */

	/* 然后我们要发送将地址发送给另外一个进程 */
	CMD cmd = kPort;
	Write(commufd_, &cmd, sizeof(cmd)); /* 传递命令 */
	Write(commufd_, &port, sizeof(port)); /* 传递端口号 */
	size_t len = strlen(ipAddr);
	printf("ip is %s\n", ipAddr);
	Write(commufd_, ipAddr, len); /* 传递ip地址 */
	Reply("%d PORT command successful. Consider using PASV.\r\n", FTP_PORTOK);
}

void CmdHandle::CDUP() { /* 进入上一个文件夹 */
	if (chdir("..") < 0) {
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

void CmdHandle::REST() {
	/* 这次要实现的一个比较有意思的功能是断点续传 */
	resumePoint_ = str2longlong(argv_.c_str());
	Reply("%d we will transfer the file from the position we got!\r\n", FTP_RESTOK);
}


void CmdHandle::RETR() { /* RETR命令用于获取数据文件 */
	Connection conn = GetConnect(); /* 获得连接 */
	int fd = open(argv_.c_str(), O_RDONLY, NULL); /* 打开文件 */
	if (fd < 0) {
		Reply("%d Open local file fail.\r\n", FTP_FILEFAIL);
		return;
	}

	/* 开始传送文件 */
	char text[1024] = { 0 };
	struct stat fileInfo;
	size_t size;
	{
		FileRDLock lock(fd); /* 读锁,如果不能读的话,会一直阻塞 */
		fstat(fd, &fileInfo);

		if (!S_ISREG(fileInfo.st_mode)) { /* 判断是否为普通文件,设备文件不能下载 */
			Reply("%d It is not a regular file.\r\n", FTP_FILEFAIL);
			return;
		}

		if (mode_ == binary) 
			sprintf(text, "Opening BINARY mode data connection for %s (%lld bytes).",
				argv_.c_str(), (long long)fileInfo.st_size);
		else 
			sprintf(text, "Opening ASCII mode data connection for %s (%lld bytes).",
				argv_.c_str(), (long long)fileInfo.st_size);

		Reply("%d %s\r\n", FTP_DATACONN, text);

		// 读取文件内容,写入套接字
		size = fileInfo.st_size;
		if (resumePoint_ != 0) {
			Lseek(fd, resumePoint_, SEEK_SET); /* 重定位文件 */
			size -= resumePoint_; /* 需要传送的字节的数目 */
		}

		barrier_.StartTimer(); /* 开始计时 */
		while (size > 0) {
			int sended = sendfile(conn->GetFd(), fd, NULL, bytesPerTime); /* 每次发送一点 */
			if (sended == -1) {
				break;
			}
			barrier_.limitSpeed(SpeedBarrier::maxDownloadSpeed, sended); /* 开始限速 */
			size -= sended;
		}
	}
	if (size == 0)
		Reply("%d Transfer complete.\r\n", FTP_TRANSFEROK);
	else {
		Reply("%d Transfer failed.\r\n", FTP_BADSENDNET); /* 连接关闭,放弃传输 */
	}
		

	utility::Close(fd); /* 关闭文件 */
	resumePoint_ = 0; /* 文件已经传送完毕了,要将断点复原 */
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
	Connection conn = GetConnect();
	/* store的话代表不用append */
	int fd = Open(argv_.c_str(), O_CREAT | O_WRONLY, 0666);
	char buf[10240];
	State state;

	{
		FileWRLock lock(fd); /* 对文件加写锁 */
		/*
		 * 在对文件进行续传的时候,对方会先发送rest命令,传递开始传送的位置.也就是我们这里的resumePoint_.
		 */
		if (resumePoint_ == 0) { 
			ftruncate(fd, 0); /* 清空文件内容 */
			Lseek(fd, 0, SEEK_SET); /* 定位到文件的开头 */
		}
		else {
			Lseek(fd, resumePoint_, SEEK_SET); /* 定位到文件的偏移位置 */
		}

		Reply("%d OK to send data.\r\n", FTP_DATACONN);

		/* 读取数据,写入本地文件 */
		barrier_.StartTimer(); /* 开始计时 */
		for (; ; ) {
			int res = read(conn->GetFd(), buf, sizeof(buf));
			if (res == -1) {
				if (errno == EINTR)
					continue;
				else {
					state = ReadError;
					break;
				}

			}
			else if (res == 0) { /* 对方关闭了连接 */
				break;
			}
			if (write(fd, buf, res) != res) { /* 往文件中写入数据 */
				state = WriteError;
				break;
			}
			barrier_.limitSpeed(SpeedBarrier::maxUploadSpeed, res); /* 对速度进行限制 */
		}
	}
	Close(fd); /* 关闭文件 */

	if (state == ReadError) { /* 读出错 */
		Reply("%d Fail to read from network stream.\r\n", FTP_BADSENDNET);
	} 
	else if (state == WriteError) {
		Reply("%d Fail to write to local file.\r\n", FTP_BADSENDFILE);
	}
	else {
		Reply("%d Transfer complete.\r\n", FTP_TRANSFEROK); /* 文件传送成功 */
	}
	
}

CmdHandle::Connection CmdHandle::GetConnect() { /* 得到一个连接 */
	int datafd = 0;
	CMD cmd = kExpectFd;
	State state;
	Write(commufd_, &cmd, sizeof(cmd));
	Read(commufd_, &state, sizeof(state)); /* 读取状态信息 */
	if (Success == state) {
		datafd = recvfd(commufd_); /* 接收fd,注意,这个文件描述符在不同进程间进行传递 */
	}
	return boost::make_shared<Conn>(datafd);
}


void CmdHandle::DELE() {
	if (unlink(argv_.c_str()) < 0) {
		Reply("%d Delete operation fail.\r\n", FTP_NOPERM);
		return;
	}
	Reply("%d Delete operation successful.\r\n", FTP_DELEOK);
}

void CmdHandle::MKD() { /* 创建一个文件夹 */
	if (mkdir(argv_.c_str(), 0777) < 0) {
		Reply("%d Create directory operation fail.\r\n", FTP_FILEFAIL);
		return;
	}
	Reply("%d Directory successfully created!\r\n", FTP_MKDIROK);
}

void CmdHandle::NOOP() {
	/* NOOP指令只要发送ok就行了! */
	Reply("%d NOOP OK.\r\n", FTP_NOOPOK);
}


void CmdHandle::SITE() {
	/* 首先对参数进行切分 */
	static std::string argv[4]; /* 最多支持4个参数 */
	std::string::iterator start = argv_.begin();
	std::string::iterator end = argv_.end();
	std::string::iterator it;
	int count = 0; /* 参数的个数 */
	while ((it = std::find(start, end, ' ')) != end) {
		argv[count].assign(start, it); /* 解析到一个参数 */
		start = it + 1; /* 去掉空格 */
		count += 1;
	}
	argv[count].assign(start, end);
	count += 1;
	
	/*
	 * 这些命令应该不会成功,因为这个程序并没有在root下运行.
	 */

	if ((argv[0] == "CHMOD") && count == 3) {
		/*SITE CHMOD <perm> <file>*/
		Chmod(atoi(argv[1].c_str()), argv[2].c_str());
	}
	else if ((argv[0] == "UMASK") && count == 2) {
		/* SITE UMASK [umask] */
		Umask(atoi(argv[1].c_str()));
	}
	else
		Reply("%d unknown site command.\r\n", FTP_BADCMD);
}

void CmdHandle::Umask(unsigned int mask) {
	umask(mask); /* 这个函数总是会成功的 */
	Reply("%d umask ok!\r\n", FTP_UMASKOK);

}
void CmdHandle::Chmod(unsigned int perm, const char* filename) {
	if (chmod(filename, perm) < 0) {
		Reply("%d SITE CHMOD command fail.\r\n", FTP_BADCMD);
		return;
	}
	Reply("%d SITE CHMOD Command OK.\r\n");
}