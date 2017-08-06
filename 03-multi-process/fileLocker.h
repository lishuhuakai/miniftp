#ifndef _FILE_LOCKER_H_
#define _FILE_LOCKER_H_
#include <sys/file.h>
#include <boost/noncopyable.hpp>

/* 写锁 */
class  FileWRLock : boost::noncopyable
{
public:
	inline FileWRLock(int fd)
		: fd_(fd)
	{
		writew_lock(fd_, SEEK_SET, 0, 0);
	}
	inline ~FileWRLock() {
		un_lock(fd_, SEEK_SET, 0, 0);
	}
private:
	int fd_;
};

/* 读锁 */
class FileRDLock : boost::noncopyable 
{
public:
	inline FileRDLock(int fd)
		: fd_(fd)
	{
		readw_lock(fd_, SEEK_SET, 0, 0);
	}
	inline ~FileRDLock()
	{
		un_lock(fd_, SEEK_SET, 0, 0);
	}
private:
	int fd_;
};


#endif

