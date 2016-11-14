#ifndef _SIGNAL_HANDLE_H_
#define _SIGNAL_HANDLE_H_

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <signal.h>
#include <map>
#include <utility>
#include "csapp.h"
/*-
* 原生的c语言信号处理函数是在简陋,简直不能忍,所以在这个类里面,我要做一下简单的封装.
*/

class SignalHandle : boost::noncopyable
{
public:
	using handler = boost::function<void()>; /* 处理函数 */
	using Router = std::map<int, handler>; /* 路由 */

	SignalHandle();

	void addSigHandle(int signo, handler&& func);
	void BlockSigno(int signo); /* 阻塞某个信号 */

	handler& GetHandler(int signo) {
		return router_[signo];
	}
private:
	Router router_;
};

#endif
