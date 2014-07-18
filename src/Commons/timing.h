#include <ctime>

#ifndef __TIMING_H__
#define __TIMING_H__

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


#endif