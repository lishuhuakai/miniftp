#ifndef _SPEED_BARRIER_H_
#define _SPEED_BARRIER_H_
#include <boost/noncopyable.hpp>
#include "TimeStamp.h"

/* 下载或者上传速度限制器 */
class SpeedBarrier : boost::noncopyable
{
public:
	void StartTimer()
	{
		start_ = Timestamp::now();
	}
	void limitSpeed(int64_t maxSpeed, size_t bytesTransed);
	static int64_t maxDownloadSpeed; /* 最大的下载速度 */
	static int64_t maxUploadSpeed; /* 最大的上传速度 */
private:
	void nanoSleep(int64_t microSeconds);
private:
	Timestamp start_; /* 开始的时间 */
};
#endif
