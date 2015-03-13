#include <cstdio>
#include <ctime>

#include "../util.h"

#ifndef __DIAG_H__
#define __DIAG_H__

struct Diag
{
	Timer HogComputation;
	Timer HofComputation;
	Timer MbhComputation;
	Timer InterpolationHOFMBH;
	Timer InterpolationHOG;

	Timer DescriptorComputation;
	Timer DescriptorQuerying;

	Timer HofQuerying;
	Timer MbhQuerying;
	Timer HogQuerying;

	Timer Everything;
	Timer ReadingAndDecoding;
	Timer Writing;

	int CallsComputeDescriptor;
	int SkippedFrames;

	Diag() : CallsComputeDescriptor(0), SkippedFrames(0) {}

	void Print(int frameCount)
	{
		log("Reading (sec): %.2lf", ReadingAndDecoding.TotalInSeconds());

		log("Interp (sec): {total: %.2lf, HOG: %.2lf, HOFMBH: %.2lf}",
			InterpolationHOFMBH.TotalInSeconds() + InterpolationHOG.TotalInSeconds(),
			InterpolationHOG.TotalInSeconds(),
			InterpolationHOFMBH.TotalInSeconds());
		
		log("IntHist (sec): {total: %.2lf, HOG: %.2lf, HOF: %.2lf, MBH: %.2lf}", 
			DescriptorComputation.TotalInSeconds(),
			HogComputation.TotalInSeconds(),
			HofComputation.TotalInSeconds(),
			MbhComputation.TotalInSeconds());
		
		log("Desc (sec): {total: %.2lf, HOG: %.2lf, HOF: %.2lf, MBH: %.2lf}",
			DescriptorQuerying.TotalInSeconds(),
			HogQuerying.TotalInSeconds(),
			HofQuerying.TotalInSeconds(),
			MbhQuerying.TotalInSeconds());

		log("Writing (sec): %.2lf", Writing.TotalInSeconds());

		double totalWithoutWriting = Everything.TotalInSeconds() - Writing.TotalInSeconds();
		log("Total (sec): %.2lf", totalWithoutWriting);
		log("Total (with writing, sec): %.2lf", Everything.TotalInSeconds());

		log("Fps: %.2lf", frameCount / totalWithoutWriting);
		log("Calls.ComputeDescriptor: %d", CallsComputeDescriptor);
		log("Frames.Skipped: %d", SkippedFrames);
	}
} TIMERS;

#endif
