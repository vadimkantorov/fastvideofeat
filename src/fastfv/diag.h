#include <cstdio>
#include <vector>

#include "../util.h"
#include "common.h"

using namespace std;

#ifndef __DIAG_H__
#define __DIAG_H__

struct Diag
{
	Timer Vocab, Reading, Writing, FLANN, Assigning, Copying, Total, ComputeGamma, UpdateFv;
	int OpCount, DescriptorCnt;

	void Print(vector<Part>& parts)
	{
		log("Vocab reading and construction (sec): %.2lf", Vocab.TotalInSeconds());
		log("Reading (sec): %.2lf", Reading.TotalInSeconds());
		log("Writing (sec): %.2lf", Writing.TotalInSeconds());
		log("Flann (sec): %.2lf", FLANN.TotalInSeconds());
		log("Assigning (sec): %.2lf", Assigning.TotalInSeconds());
		log("ComputeGamma (sec): %.2lf", ComputeGamma.TotalInSeconds());
		log("UpdateFv (sec): %.2lf", UpdateFv.TotalInSeconds());

		log("Copying (sec): %.2lf", Copying.TotalInSeconds());
		log("Total (sec): %.2lf", Total.TotalInSeconds());
		log("Ops: %d", OpCount);
		log("Descriptors: %d", DescriptorCnt);
		fprintf(stderr, "Part sizes: [");
		for(int i = 0; i < parts.size(); i++)
			fprintf(stderr, "%d, ", parts[i].Size);
		log("]");
	}

	Diag() : OpCount(0), DescriptorCnt(0) {}
};
Diag TIMERS;

#endif
