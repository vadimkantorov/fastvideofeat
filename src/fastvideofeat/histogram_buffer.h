#include <vector>
#include <utility>

#include <opencv2/flann/kmeans_index.h>
#include <opencv/cv.h>

#include "integral_transform.h"
#include "diag.h"

using namespace cv;
using namespace std;

#ifndef __HISTOGRAM_BUFFER_H__
#define __HISTOGRAM_BUFFER_H__

struct HistogramBuffer
{
	vector<pair<Mat, Mat > > currentStack;
	vector<Mat> gluedIntegralTransforms;
	DescInfo descInfo;
	int tStride;

	HistogramBuffer(DescInfo descInfo, int tStride) : 
		descInfo(descInfo),
		tStride(tStride)
	{
		gluedIntegralTransforms.resize(descInfo.ntCells);
	}

	void AddUpCurrentStack()
	{
		Mat cumulativeIntegralTransform;
		for(int i = 0; i < currentStack.size(); i++)
		{
			Mat integralTransform = BuildOrientationIntegralTransform(descInfo, currentStack[i].first, currentStack[i].second);
			if(i == 0)
				cumulativeIntegralTransform = integralTransform;
			else
				cumulativeIntegralTransform += integralTransform;
		}

		rotate(gluedIntegralTransforms.begin(), ++gluedIntegralTransforms.begin(), gluedIntegralTransforms.end());
		gluedIntegralTransforms.back() = cumulativeIntegralTransform / tStride;
		currentStack.clear();
	}

	void QueryPatchDescriptor(Rect rect, float* res)
	{
		descInfo.ResetPatchDescriptorBuffer(res);
		for(int iT = 0; iT < descInfo.ntCells; iT++)
			ComputeDescriptor(gluedIntegralTransforms[iT], rect, descInfo, res + iT*descInfo.dim);
	}

	void Update(Mat dx, Mat dy)
	{
		currentStack.push_back(make_pair(dx, dy));	
	}
};

struct HofMbhBuffer
{
	Size frameSizeAfterInterpolation;
	bool print;
	bool AreDescriptorsReady;
	vector<int> effectiveFrameIndices;
	int tStride;
	int ntCells;
	double fScale;

	HistogramBuffer hog;
	HistogramBuffer hof;
	HistogramBuffer mbhX;
	HistogramBuffer mbhY;

	Mat patchDescriptor;

	float* hog_patchDescriptor;
	float* hof_patchDescriptor;
	float* mbhX_patchDescriptor;
	float* mbhY_patchDescriptor;

	DescInfo hogInfo, hofInfo, mbhInfo;

	void CreatePatchDescriptorPlaceholder(DescInfo& hogInfo, DescInfo& hofInfo, DescInfo& mbhInfo)
	{
		int size = (hogInfo.enabled ? hogInfo.fullDim : 0) 
			+ (hofInfo.enabled ? hofInfo.fullDim : 0)
			+ (mbhInfo.enabled ? 2*mbhInfo.fullDim : 0);
		patchDescriptor.create(1, size, CV_32F);
		float* begin = patchDescriptor.ptr<float>();

		int used = 0;
		if(hogInfo.enabled)
		{
			hog_patchDescriptor = begin + used;
			used += hogInfo.fullDim;
		}
		if(hofInfo.enabled)
		{
			hof_patchDescriptor = begin + used;
			used += hofInfo.fullDim;
		}
		if(mbhInfo.enabled)
		{
			mbhX_patchDescriptor = begin + used;
			used += mbhInfo.fullDim;

			mbhY_patchDescriptor = begin + used;
			used += mbhInfo.fullDim;
		}
	}

	HofMbhBuffer(
		DescInfo hogInfo, 
		DescInfo hofInfo, 
		DescInfo mbhInfo,
		int ntCells, 
		int tStride, 
		Size frameSizeAfterInterpolation, 
		double fScale, 
		bool print = false)
		: 
		frameSizeAfterInterpolation(frameSizeAfterInterpolation), 
		ntCells(ntCells),
		tStride(tStride),
		fScale(fScale),
		print(print),

		hof(hofInfo, tStride),
		mbhX(mbhInfo, tStride),
		mbhY(mbhInfo, tStride),
		hog(hogInfo, tStride),

		hog_patchDescriptor(NULL), 
		hof_patchDescriptor(NULL),
		mbhX_patchDescriptor(NULL),
		mbhY_patchDescriptor(NULL),

		hogInfo(hogInfo),
		hofInfo(hofInfo),
		mbhInfo(mbhInfo),
		AreDescriptorsReady(false)
	{
		CreatePatchDescriptorPlaceholder(hogInfo, hofInfo, mbhInfo);
	}

	void Update(Frame& frame)
	{
		if(hofInfo.enabled)
		{
			TIMERS.HofComputation.Start();
			if(!frame.WarpDx.empty() && !frame.WarpDy.empty())
				hof.Update(frame.WarpDx, frame.WarpDy);
			else
				hof.Update(frame.Dx, frame.Dy);
			TIMERS.HofComputation.Stop();
		}

		if(mbhInfo.enabled)
		{
			TIMERS.MbhComputation.Start();
			Mat flowXdX, flowXdY, flowYdX, flowYdY;
			Sobel(frame.Dx, flowXdX, CV_32F, 1, 0, 1);
			Sobel(frame.Dx, flowXdY, CV_32F, 0, 1, 1);
			Sobel(frame.Dy, flowYdX, CV_32F, 1, 0, 1);
			Sobel(frame.Dy, flowYdY, CV_32F, 0, 1, 1);
			mbhX.Update(flowXdX, flowXdY);
			mbhY.Update(flowYdX, flowYdY);
			TIMERS.MbhComputation.Stop();
		}

		if(hogInfo.enabled)
		{
			TIMERS.HogComputation.Start();
			Mat dx, dy;
			Sobel(frame.RawImage, dx, CV_32F, 1, 0, 1);
			Sobel(frame.RawImage, dy, CV_32F, 0, 1, 1);
			hog.Update(dx, dy);
			TIMERS.HogComputation.Stop();
		}

		effectiveFrameIndices.push_back(frame.PTS);
		AreDescriptorsReady = false;
		if(effectiveFrameIndices.size() % tStride == 0)
		{
			if(hofInfo.enabled)
			{
				TIMERS.HofComputation.Start();
				hof.AddUpCurrentStack();
				TIMERS.HofComputation.Stop();
			}

			if(mbhInfo.enabled)
			{
				TIMERS.MbhComputation.Start();
				mbhX.AddUpCurrentStack();
				mbhY.AddUpCurrentStack();
				TIMERS.MbhComputation.Stop();
			}
			if(hogInfo.enabled)
			{
				TIMERS.HogComputation.Start();
				hog.AddUpCurrentStack();
				TIMERS.HogComputation.Stop();
			}

			AreDescriptorsReady = effectiveFrameIndices.size() >= ntCells * tStride;
		}
	}

	void PrintFileHeader()
	{
		printf("#descr = ");
		if(hogInfo.enabled)
			printf("hog (%d) ", hogInfo.fullDim);
		if(hofInfo.enabled)
			printf("hof (%d) ", hofInfo.fullDim);
		if(mbhInfo.enabled)
			printf("mbh (%d + %d)", mbhInfo.fullDim, mbhInfo.fullDim);
		printf("\n#x\ty\tpts\tStartPTS\tEndPTS\tXoffset\tYoffset\tPatchWidth\tPatchHeight\tdescr\n");
	}

	void PrintPatchDescriptorHeader(Rect rect)
	{
		int firstFrame = effectiveFrameIndices[effectiveFrameIndices.size()-ntCells*tStride];
		int lastFrame = effectiveFrameIndices.back();
		Point patchCenter(rect.x + rect.width/2, rect.y + rect.height/2);
		printf("%.2lf\t%.2lf\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t", 
			double(patchCenter.x) / frameSizeAfterInterpolation.width,
			double(patchCenter.y) / frameSizeAfterInterpolation.height,
			(firstFrame + lastFrame)/2,
			firstFrame, 
			lastFrame, 
			int(rect.x / fScale), 
			int(rect.y / fScale), 
			int(rect.width / fScale), 
			int(rect.height / fScale));
	}

	void PrintPatchDescriptor(Rect rect)
	{
		TIMERS.DescriptorQuerying.Start();
		if(hofInfo.enabled)
		{
			TIMERS.HofQuerying.Start();
			hof.QueryPatchDescriptor(rect, hof_patchDescriptor);
			TIMERS.HofQuerying.Stop();
		}
		if(mbhInfo.enabled)
		{
			TIMERS.MbhQuerying.Start();
			mbhX.QueryPatchDescriptor(rect, mbhX_patchDescriptor);
			mbhY.QueryPatchDescriptor(rect, mbhY_patchDescriptor);
			TIMERS.MbhQuerying.Stop();
		}
		if(hogInfo.enabled)
		{
			TIMERS.HogQuerying.Start();
			hog.QueryPatchDescriptor(rect, hog_patchDescriptor);
			TIMERS.HogQuerying.Stop();
		}
		TIMERS.DescriptorQuerying.Stop();
		
		if(print)
		{
			TIMERS.Writing.Start();
			PrintPatchDescriptorHeader(rect);
			/*if(printQuantized)
			{
				quantizer.Quantize(patchDescriptor, quantized);
				PrintIntegerArray(quantized);
			}
			else
			{*/
				PrintFloatArray(patchDescriptor);
			//}
			
			printf("\n");
			TIMERS.Writing.Stop();
			
		}
	}

	void PrintFullDescriptor(int blockWidth, int blockHeight, int xStride, int yStride)
	{
		for(int xOffset = 0; xOffset + blockWidth < frameSizeAfterInterpolation.width; xOffset += xStride)
		{
			for(int yOffset = 0; yOffset + blockHeight < frameSizeAfterInterpolation.height; yOffset += yStride)
			{
				Rect rect(xOffset, yOffset, blockWidth, blockHeight);
				PrintPatchDescriptor(rect);
			}
		}
	}
};

#endif
