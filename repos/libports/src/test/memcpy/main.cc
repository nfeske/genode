#include <stdio.h>
#include <time.h>
#include <stdlib.h>


struct Duration { unsigned long usecs; };


struct Time
{
	timespec _timespec { 0, 0 };

	Time()
	{
		clock_gettime(CLOCK_REALTIME, &_timespec);
	}

	Time(timespec timespec) : _timespec(timespec) { }

	void print() const
	{
		printf("secs=%ld nsecs=%ld\n",
		       (long)_timespec.tv_sec, (long)_timespec.tv_nsec);
	}

	static Duration duration(Time t1, Time t2)
	{
		auto usecs = [&] (timespec ts) {
			return 1000UL*1000*(ts.tv_sec % 1000) + ts.tv_nsec/1000; };

		return Duration { usecs(t2._timespec) - usecs(t1._timespec) };
	}
};


static inline void *bytewise_memcpy(void *dst, const void *src, size_t size)
{
	char *d = (char *)dst, *s = (char *)src;

	/* copy eight byte chunks */
	for (size_t i = size >> 3; i > 0; i--, *d++ = *s++,
	                                       *d++ = *s++,
	                                       *d++ = *s++,
	                                       *d++ = *s++,
	                                       *d++ = *s++,
	                                       *d++ = *s++,
	                                       *d++ = *s++,
	                                       *d++ = *s++);

	/* copy left over */
	for (size_t i = 0; i < (size & 0x7); i++, *d++ = *s++);

	return dst;
}


int main(int, char **)
{
	size_t const buf_size = 8*1024*1024;

	void * const from_buf = malloc(buf_size);
	void * const to_buf   = malloc(buf_size);

	Time start;

	size_t kib = 0;
	for (unsigned i = 0; i < 1000; i++) {
		bytewise_memcpy(to_buf, from_buf, buf_size);
		kib += buf_size/1024;
	}

	Time end;

	Duration duration = Time::duration(start, end);

	printf("copied %ld KiB in %ld usecs ", kib, duration.usecs);
	printf("(%ld MiB/sec)\n", 1000*kib/duration.usecs);




}
