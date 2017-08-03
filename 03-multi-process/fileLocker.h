#ifndef _FILE_LOCKER_H_
#define _FILE_LOCKER_H_
#include <sys/file.h>
#include <boost/noncopyable.hpp>

/* Ð´Ëø */
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

/* ¶ÁËø */
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

