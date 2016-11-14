#ifndef _CMD_H_
#define _CMD_H_
namespace utility {
	enum CMD { /* 用于进程件的命令传递 */
		kExpectPort, /* 期望获得端口号 */
		kExpectFd, /* 希望接收到对方的连接 */
		kGotPort, /* 得到了端口号 */
		kGotFd,
		kExpectConn,
		kList, /* List命令 */
		kCDUP,
		kCWD,
	};

	enum State {
		Error, /* 出错 */
		Success, /* 成功 */
		ReadError,
		WriteError,
		DirOpenError,
		SendListOk,
	};
}

#endif