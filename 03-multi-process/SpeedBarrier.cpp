#include "SpeedBarrier.h"
#include <time.h>
#include <errno.h>

int64_t SpeedBarrier::maxDownloadSpeed = 1024 * 1024;
int64_t SpeedBarrier::maxUploadSpeed = 1024 * 1024;

void SpeedBarrier::limitSpeed(int64_t maxSpeed, size_t bytesTransed) {
	/* 开始限制速度 */
	Timestamp now = Timestamp::now();
	int64_t timePassed = now.microSecondsSinceEpoch() - start_.microSecondsSinceEpoch();
	int64_t speedNow = bytesTransed / (static_cast<double>(timePassed) / Timestamp::kMicroSecondsPerSecond); /* 求出每秒钟传送的字节数 */
	if (speedNow > maxSpeed) {
		/* 然后我们就必须休眠 */
		int64_t diff = speedNow - maxSpeed; /* 求出两者之间的差值 */
		int64_t sleepMicroSeconds = (diff / static_cast<double>(maxSpeed)) * timePassed;
		nanoSleep(sleepMicroSeconds);
		//usleep(sleepMicroSeconds); // usleep要求参数小于1000000,也就是1秒,这是不现实的.
	}
	start_ = Timestamp::now(); /* 重新计时 */
}

void SpeedBarrier::nanoSleep(int64_t microSeconds) {
	struct timespec timeSpan;
	timeSpan.tv_sec = microSeconds / Timestamp::kMicroSecondsPerSecond;
	// 1微秒等于1000纳秒
	timeSpan.tv_nsec = (microSeconds - timeSpan.tv_sec * Timestamp::kMicroSecondsPerSecond) * 1000; /* 纳秒级别的精度 */
	
	int res;
	do {
		res = nanosleep(&timeSpan, &timeSpan);
	} while (res == -1 && errno == EINTR); /* 可能被信号中断,所以要用do while格式 */
}