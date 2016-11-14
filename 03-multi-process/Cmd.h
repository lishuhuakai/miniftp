#ifndef _CMD_H_
#define _CMD_H_
namespace utility {
	enum CMD { /* 用于进程件的命令传递 */
		kExpectPort, /* 期望获得端口号 */
		kExpectFd, /* 希望接收到对方的连接 */
		kList, /* list命令 */
		kPort, /* port命令 */
		kClose, /* 关闭 */
	};

	enum State {
		Error,
		Success,
		ReadError,
		WriteError,
	};
}

#endif