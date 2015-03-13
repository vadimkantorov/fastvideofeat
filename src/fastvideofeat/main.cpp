#include <fstream>
#include <string>
#include <cstdio>
#include <algorithm>
#include <opencv/cv.h>
#include <opencv/cxcore.h>

#include "../util.h"
#include "video.h"
#include "descriptors.h"
#include "diag.h"

#include <iterator>
#include <vector>

using namespace std;
using namespace cv;

struct Options
{
	string VideoPath;
	bool HogEnabled, HofEnabled, MbhEnabled;
	bool Dense;
	bool Interpolation;

	vector<int> GoodPts;

	Options(int argc, char* argv[])
	{
		HogEnabled = HofEnabled = MbhEnabled = true;
		Dense = false;
		Interpolation = true;
		for(int i = 1; i < argc; i++)
		{
			if(strcmp(argv[i], "--disableHOG") == 0)
				HogEnabled = false;
			else if(strcmp(argv[i], "--disableHOF") == 0)
				HofEnabled = false;
			else if(strcmp(argv[i], "--disableMBH") == 0)
				MbhEnabled = false;
			else if(strcmp(argv[i], "-f") == 0)
			{
				int b, e;
				sscanf(argv[i+1], "%d-%d", &b, &e);
				for(int j = b; j <= e; j++)
					GoodPts.push_back(j);
				i++;
			}
			else
				VideoPath = string(argv[i]);
		}
	
		if(!ifstream(VideoPath.c_str()).good())
			throw runtime_error("Video doesn't exist or can't be opened: " + VideoPath);
	}
};

int main(int argc, char* argv[])
{
	Options opts(argc, argv);
	setNumThreads(1);

	const int nt_cell = 3;
	const int tStride = 5;
	vector<Size> patchSizes;
	patchSizes.push_back(Size(32, 32));
	patchSizes.push_back(Size(48, 48));

	DescInfo hofInfo(8+1, true, nt_cell, opts.HofEnabled);
	DescInfo mbhInfo(8, false, nt_cell, opts.MbhEnabled);
	DescInfo hogInfo(8, false, nt_cell, opts.HogEnabled);

	TIMERS.ReadingAndDecoding.Start();
	FrameReader rdr(opts.VideoPath, hogInfo.enabled);
	TIMERS.ReadingAndDecoding.Stop();

	Size frameSizeAfterInterpolation = 
		opts.Interpolation
			? Size(2*rdr.DownsampledFrameSize.width - 1, 2*rdr.DownsampledFrameSize.height - 1)
			: rdr.DownsampledFrameSize;
	int cellSize = rdr.OriginalFrameSize.width / frameSizeAfterInterpolation.width;
	double fscale = 1 / 8.0;

	log("Input video: %s", opts.VideoPath.c_str());
	log("Enabled descriptors: {HOG: %s, HOF: %s, MBH: %s}", opts.HogEnabled ? "yes" : "no", opts.HofEnabled ? "yes" : "no", opts.MbhEnabled ? "yes" : "no");
	fprintf(stderr, "Frame restrictions: [");
	for(int i = 0; i < opts.GoodPts.size(); i++)
		fprintf(stderr, "%d, ", opts.GoodPts[i]);
	log("]");
	log("Frame count: %d", rdr.FrameCount);
	log("Original frame size: %dx%d", rdr.OriginalFrameSize.width, rdr.OriginalFrameSize.height);
	log("Downsampled: %dx%d", rdr.DownsampledFrameSize.width, rdr.DownsampledFrameSize.height);
	log("After interpolation: %dx%d", frameSizeAfterInterpolation.width, frameSizeAfterInterpolation.height);
	log("CellSize: %d", cellSize);

	HofMbhBuffer buffer(hogInfo, hofInfo, mbhInfo, nt_cell, tStride, frameSizeAfterInterpolation, fscale, rdr.FrameCount, true);
 	buffer.PrintFileHeader();

	TIMERS.Everything.Start();
	Mat prevRawImageGray, currentRawImageGray;
	while(true)
	{
		Frame frame = rdr.Read();
		if(frame.PTS == -1)
			break;

		log("# read frame pts=%d, mvs=%s, type=%c", frame.PTS, frame.NoMotionVectors ? "no" : "yes", frame.PictType);

		if(opts.GoodPts.empty() || count(opts.GoodPts.begin(), opts.GoodPts.end(), frame.PTS) == 1)
		{
			TIMERS.DescriptorComputation.Start();
			
			if(frame.NoMotionVectors || (hogInfo.enabled && frame.RawImage.empty()))
			{
				TIMERS.SkippedFrames++;
				continue;
			}

			frame.Interpolate(frameSizeAfterInterpolation, fscale);
			//buffer.Update(frame, 1 / fscale * 1 / fscale);
			buffer.Update(frame, 1);
			TIMERS.DescriptorComputation.Stop();
		
			if(buffer.AreDescriptorsReady)
			{
				for(int k = 0; k < patchSizes.size(); k++)
				{
					int blockWidth = patchSizes[k].width / cellSize;
					int blockHeight = patchSizes[k].height / cellSize;
					int xStride = opts.Dense ? 1 : blockWidth / 2;
					int yStride = opts.Dense ? 1 : blockHeight / 2;
					buffer.PrintFullDescriptor(blockWidth, blockHeight, xStride, yStride);
				}
			}
		}
	}
	TIMERS.Everything.Stop();
	TIMERS.Print(rdr.FrameCount);
 }
