#ifndef _CMD_HANDLE_H_
#define _CMD_HANDLE_H_
#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <map>
#include <string>
#include "Connect.h"
#include "Cmd.h"
#include "SpeedBarrier.h"
#include "SignalHandle.h"
#include "buff.h" /* 读入还是要一个缓冲区的比较好! */

class CmdHandle : boost::noncopyable /* 这个类主要用于处理ftp的命令 */
{
public:
	using Handler = boost::function<void()>;
	using Router = std::map<std::string, Handler>;
	using Connection = boost::shared_ptr<Conn>;
	CmdHandle(int cmdfd, int commufd);
	~CmdHandle();
public:
	void Handle();
private:
	enum Mode {
		ascii, /* ascii文本格式 */
		binary, /* 二进制格式 */
	};
	Mode mode_;
private:
	void FEAT();
	void SYST(); /* 操作系统类型 */
	void PWD(); /* 当前目录 */
	void OPTS(); /* 用于调整一些选项 */
	void CWD();
	void PASV();
	void LIST(); /* 获得文件信息 */
	void PORT();
	void CDUP(); /* 进入上一个文件夹 */
	void SIZE(); /* 获取文件的大小 */
	void RETR(); /* 获取某个文件 */
	void TYPE(); /* 获取啥? */
	void RMD(); /* 删除掉某个文件夹 */
	void STOR(); /* 上传文件 */
	void MKD(); /* 新建文件夹 */
	void DELE(); /* 删除某个文件 */
	void QUIT(); /* 关闭 */
	void REST(); /* 实现断点续传 */
	void ABOR();
	void NOOP();
	void RNFR(); /* 重命名开始 */
	void RNTO(); /* 开始重命名 */
	void SITE(); /* site命令 */
private:
	void HandleUrg(); /* 接收到UEG信号的处理函数 */
	void Chmod(unsigned int perm, const char* filename);
	void Umask(unsigned int mask);
private:
	void Reply(const char *format, ...); /* 发送回复信息 */
	bool GetLine(char* buf, size_t len);
	void login(); /* 处理登陆信息 */
	utility::State Sendlist(bool detail);
	Connection GetConnect();
	void Parsing(char* cmdLine);
private:
	static const int bytesPerTime = 1024 * 1024; /* 每次发送的字节数目1Mb */
	int cmdfd_; /* 此外,我们还需要一条管道 */
	int commufd_; /* 用于和另外一个进程进行交流 */
	Router router_; /* 实现命令到函数的映射 */
	bool stop_; /* 是否退出 */
	std::string cmd_; /* 记录命令 */
	std::string argv_; /* 用来记录参数 */
	std::string name_; /* 用来记录需要重命名的文件名 */
	SpeedBarrier barrier_; /* 速度限制器 */
	long long resumePoint_; /* 断点续传的位置 */
	SignalHandle& sighanler_; /* 信号处理器 */
	Buffer buffer_; /* 读入缓冲区 */
};

#endif
