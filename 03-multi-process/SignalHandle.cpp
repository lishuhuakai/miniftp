#include "SignalHandle.h"

static SignalHandle* handle; /* dirty的代码要封装起来 */

static void sighandler(int);

static void addsig(int sig) {
	struct sigaction sa;
	bzero(&sa, sizeof(sa));
	sa.sa_handler = sighandler;
	sa.sa_flags |= SA_RESTART;
	utility::Sigfillset(&sa.sa_mask);
	sigaction(sig, &sa, NULL);
}


static void sighandler(int signo) { /* 为了防止污染整个应用,所以设置为了static类型 */
	boost::function<void()> func = handle->GetHandler(signo);
	if (func)
		func();
}

void SignalHandle::BlockSigno(int signo) { /* 阻塞掉某个信号 */
	sigset_t signal_mask;
	utility::Sigemptyset(&signal_mask);
	utility::Sigaddset(&signal_mask, signo);
	utility::Sigprocmask(SIG_BLOCK, &signal_mask, NULL);
}

void SignalHandle::addSigHandle(int signo, handler&& func) {
	router_[signo] = std::move(func);
	//utility::Signal(SIGURG, sighandler); /* 注册信号处理函数 */
	addsig(signo);
}

SignalHandle::SignalHandle() { /* 初始化函数 */
	handle = this; /* 记录下指针的值 */
}
