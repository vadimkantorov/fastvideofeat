#include <cstdio>
#include <vector>

#include "../commons/timing.h"
#include "options.h"

using namespace std;

#ifndef __DIAG_H__
#define __DIAG_H__

struct Diag
{
	Timer Vocab;
	Timer Reading;
	Timer Writing;
	Timer FLANN;
	Timer Assigning;
	Timer Copying;
	Timer Total;
	Timer ComputeGamma;
	Timer UpdateFv;
	int OpCount;
	int DescriptorCnt;

	void Print(vector<Part>& parts)
	{
		printf("#STAT Vocab reading and construction: %.2lf secs\n", Vocab.TotalInSeconds());
		printf("#STAT Reading: %.2lf secs\n", Reading.TotalInSeconds());
		printf("#STAT Writing: %.2lf secs\n", Writing.TotalInSeconds());
		printf("#STAT Flann: %.2lf secs\n", FLANN.TotalInSeconds());
		printf("#STAT Assigning: %.2lf secs\n", Assigning.TotalInSeconds());
		printf("#STAT ComputeGamma: %.2lf secs\n", ComputeGamma.TotalInSeconds());
		printf("#STAT UpdateFv: %.2lf secs\n", UpdateFv.TotalInSeconds());

		printf("#STAT Copying: %.2lf secs\n", Copying.TotalInSeconds());
		printf("#STAT Total: %.2lf secs\n", Total.TotalInSeconds());
		printf("#STAT Ops: %d\n", OpCount);
		printf("#STAT Descriptors: %d\n", DescriptorCnt);
		printf("#STAT Part sizes: ");
		for(int i = 0; i < parts.size(); i++)
			printf("%d ", parts[i].Size());
		printf("\n");

	}

	Diag() : OpCount(0), DescriptorCnt(0) {}
};
Diag TIMERS;

#endif
