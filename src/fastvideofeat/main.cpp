#include <string>
#include <cstdio>
#include <algorithm>
#include <opencv/cv.h>
#include <opencv/cxcore.h>

#include "motion_vector_file_utils.h"
#include "../commons/log.h"
#include "frame_reader.h"
#include "histogram_buffer.h"
#include "desc_info.h"
#include "options.h"
#include "diag.h"

#include <iterator>
#include <vector>

using namespace std;
using namespace cv;

int main(int argc, char* argv[])
{
	Options opts(argc, argv);

	const int nt_cell = 3;
	const int tStride = 5;
	vector<Size> patchSizes;
	patchSizes.push_back(Size(32, 32));
	patchSizes.push_back(Size(48, 48));

	DescInfo hofInfo(8+1, true, nt_cell, opts.HofEnabled);
	DescInfo mbhInfo(8, false, nt_cell, opts.MbhEnabled);
	DescInfo hogInfo(8, false, nt_cell, opts.HogEnabled);

	TIMERS.Reading.Start();
	FrameReader rdr(opts.VideoPath, hogInfo.enabled);
	TIMERS.Reading.Stop();

	Size frameSizeAfterInterpolation = 
		opts.Interpolation
			? Size(2*rdr.DownsampledFrameSize.width - 1, 2*rdr.DownsampledFrameSize.height - 1)
			: rdr.DownsampledFrameSize;
	int cellSize = rdr.OriginalFrameSize.width / frameSizeAfterInterpolation.width;
	double fscale = 1 / 8.0;

	log("Frame count:\t%d", rdr.FrameCount);
	log("Original frame size:\t%dx%d", rdr.OriginalFrameSize.width, rdr.OriginalFrameSize.height);
	log("Downsampled:\t%dx%d", rdr.DownsampledFrameSize.width, rdr.DownsampledFrameSize.height);
	log("After interpolation:\t%dx%d", frameSizeAfterInterpolation.width, frameSizeAfterInterpolation.height);
	log("CellSize:\t%d", cellSize);

	HofMbhBuffer buffer(hogInfo, hofInfo, mbhInfo, nt_cell, tStride, frameSizeAfterInterpolation, fscale, true);
 	buffer.PrintFileHeader();

	TIMERS.Everything.Start();
	Mat prevRawImageGray, currentRawImageGray;
	while(true)
	{
		Frame frame = rdr.Read();
		if(frame.PTS == -1)
			break;

		log("#read frame pts=%d", frame.PTS);

		if(opts.GoodPts.empty() || count(opts.GoodPts.begin(), opts.GoodPts.end(), frame.PTS) == 1)
		{
			TIMERS.DescriptorComputation.Start();
			
			if(frame.NoMotionVectors || (hogInfo.enabled && frame.RawImage.empty()))
			{
				log("#skipping");
				TIMERS.SkippedFrames++;
				continue;
			}

			frame.Interpolate(frameSizeAfterInterpolation, fscale);
			buffer.Update(frame);
			TIMERS.DescriptorComputation.Stop();
		
			if(buffer.AreDescriptorsReady)
			{
				for(int k = 0; k < patchSizes.size(); k++)
				{
					int blockWidth = patchSizes[k].width / cellSize;
					int blockHeight = patchSizes[k].height / cellSize;
					int xStride = opts.Dense ? 1 : blockWidth / 2;
					int yStride = opts.Dense ? 1 : blockHeight / 2;
					log("#cellSize: %d, blockWidth: %d, blockHeight: %d, xStride: %d, yStride: %d", cellSize, blockWidth, blockHeight, xStride, yStride);
					buffer.PrintFullDescriptor(blockWidth, blockHeight, xStride, yStride);
				}
			}
		}
	}
	TIMERS.Everything.Stop();
	TIMERS.Print(rdr.FrameCount);
 }
