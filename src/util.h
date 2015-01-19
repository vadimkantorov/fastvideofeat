#include <cstdio>
#include <cstdlib>
#include <string>
#include <opencv/cv.h>

#ifndef __UTIL_H__
#define __UTIL_H__


struct Timer
{
	clock_t before;
	clock_t total;

	Timer() : before(0), total(0) {}
	void Start()
	{
		before = clock();
	}

	void Stop()
	{
		total += clock() - before;
	}

	double TotalInSeconds()
	{
		return double(total) / CLOCKS_PER_SEC;
	}

	double TotalInMilliseconds()
	{
		double CLOCKS_PER_MSEC = CLOCKS_PER_SEC / 1000.0;
		return double(total) / CLOCKS_PER_MSEC;
	}
};

void log(const char* fmt, ...)
{
	FILE* out = stderr;
	va_list argp;
	va_start(argp, fmt);
	vfprintf(out, fmt, argp);
	va_end(argp);
	fprintf(out, "\n");
	fflush(out);
}

#endif
