#include "TimeStamp.h"
#include <sys/time.h>
#include <stdio.h>
#include <inttypes.h>

Timestamp Timestamp::now() {
	struct timeval tv;
	gettimeofday(&tv, NULL); /* NULL表示我们不需要时区的信息 */
	int64_t seconds = tv.tv_sec; /* 得到对应的秒数 */
	return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec); /* Timestamp内部是用微秒来表示的! */
}