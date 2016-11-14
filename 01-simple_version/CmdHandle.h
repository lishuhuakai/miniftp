#ifndef _CMD_HANDLE_H_
#define _CMD_HANDLE_H_
#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <map>
#include <string>
#include "Connect.h"

class CmdHandle : boost::noncopyable /* 这个类主要用于处理ftp的命令 */
{
public:
	typedef boost::function<void()> Handler;
	typedef std::map<std::string, Handler> Router;
	typedef boost::shared_ptr<Conn> Connection;
	CmdHandle(int cmdfd, int commufd);
	~CmdHandle() {}
public:
	void Handle();
private:
	enum Mode {
		ascii, /* ascii文本格式 */
		binary, /* 二进制格式 */
	};
	Mode mode_;

	enum State {
		error, /* 出错 */
		success, /* 成功 */
		readerror,
		writeerror,
	};
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
	void STOR(); /* 上传文件 */
	void MKD(); /* 新建文件夹 */
	void DELE(); /* 删除某个文件 */
	void QUIT(); /* 关闭 */
private:
	void Reply(const char *format, ...); /* 发送回复信息 */
	void GetLine(char* buf, size_t len);
	void Login(); /* 处理登陆信息 */
	State SendList(bool detail);
	Connection GetConnect();
	void Parsing(char* cmdLine);
private:
	int cmdfd_; /* 此外,我们还需要一条管道 */
	int commufd_; /* 用于和另外一个进程进行交流 */
	sockaddr_in clnaddr_;
	bool passive_;

	Router router_; /* 实现命令到函数的映射 */
	std::string cmd_; /* 记录命令 */
	std::string argv_; /* 用来记录参数 */
};

#endif
