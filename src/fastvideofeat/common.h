#include <cstdio>
#include "diag.h"
#include <opencv/cv.h>

using namespace cv;

#ifndef __COMMON_H__
#define __COMMON_H__

Mat InterpolateFrom16to8(Mat src, Size afterInterpolation, double fscale = 1)
{
	Mat dst(afterInterpolation, src.type());
	resize(src, dst, dst.size());
	return dst * fscale;
}

struct Frame
{
	Mat_<float> Dx, WarpDx;
	Mat_<float> Dy, WarpDy;
	Mat_<bool> Missing;
	Mat RawImage;
	int FrameIndex;
	int64_t PTS;
	bool NoMotionVectors;
	char PictType;

	Frame(int frameIndex, Mat dx, Mat dy, Mat missing)
		: FrameIndex(frameIndex), Dx(dx), Dy(dy), Missing(missing), NoMotionVectors(false), PTS(-1), PictType('?')
	{
	}

	Frame(int frameIndex = -1) : FrameIndex(frameIndex), NoMotionVectors(true), PTS(-1), PictType('?')
	{
	}

	static Frame Null(int frameIndex)
	{
		return Frame(frameIndex);
	}

	void Interpolate(Size afterInterpolation, double fscale)
	{
		if(!NoMotionVectors)
		{
			TIMERS.InterpolationHOFMBH.Start();
			Dx = InterpolateFrom16to8(Dx, afterInterpolation, fscale);
			Dy = InterpolateFrom16to8(Dy, afterInterpolation, fscale);

			if(!WarpDx.empty() && !WarpDy.empty())
			{
				WarpDx = InterpolateFrom16to8(WarpDx, afterInterpolation, fscale);
				WarpDy = InterpolateFrom16to8(WarpDy, afterInterpolation, fscale);
			}

			TIMERS.InterpolationHOFMBH.Stop();
		}

		if(RawImage.data)
		{
			TIMERS.InterpolationHOG.Start();
			Mat rawImageResized;
			resize(RawImage, rawImageResized, afterInterpolation);
			cvtColor(rawImageResized, RawImage, CV_BGR2GRAY);
			TIMERS.InterpolationHOG.Stop();
		}
	}
};

void PrintIntegerArray(Mat& m)
{
	int* ptr_m = m.ptr<int>();
	for(int i = 0; i < m.size().area(); i++)
	{
		printf("%d\t", ptr_m[i]);
	}
}

void PrintFloatArray(Mat& m)
{
	float* ptr_m = m.ptr<float>();
	for(int i = 0; i < m.size().area(); i++)
	{
		printf("%.6f\t", ptr_m[i]);
	}
}

void PrintDoubleArray(Mat& m)
{
	double* ptr_m = m.ptr<double>();
	for(int i = 0; i < m.size().area(); i++)
	{
		printf("%.6lf\t", ptr_m[i]);
	}
}

#endif
