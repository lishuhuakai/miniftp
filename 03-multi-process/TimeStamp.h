#ifndef _TIME_STAMP_H_
#define _TIME_STAMP_H_
#include <algorithm>

class Timestamp
{
public:
	Timestamp()
		: microSecondsSinceEpoch_(0)
	{}

	explicit Timestamp(int64_t microSecondsSinceEpochArg)
		: microSecondsSinceEpoch_(microSecondsSinceEpochArg)
	{}

	void swap(Timestamp& that)
	{
		std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
	}

	bool valid() const {
		return microSecondsSinceEpoch_ > 0;
	}

	int64_t microSecondsSinceEpoch() const {
		return microSecondsSinceEpoch_;
	}

	time_t secondsSinceEpoch() const {
		return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
	}

	static Timestamp now();
	static Timestamp invalid()
	{
		return Timestamp();
	}

	static const int kMicroSecondsPerSecond = 1000 * 1000; /* 1秒等于1000000微秒 */
private:
	int64_t microSecondsSinceEpoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
	return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
	return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

#endif
