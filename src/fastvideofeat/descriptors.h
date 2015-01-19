#include <vector>
#include <utility>

#include <opencv/cv.h>

#include "common.h"
#include "diag.h"

using namespace cv;
using namespace std;

#ifndef __HISTOGRAM_BUFFER_H__
#define __HISTOGRAM_BUFFER_H__

float FastSquareRootFloat(float number) {
    long i;
    float x, y;
    const float f = 1.5F;

    x = number * 0.5F;
    y  = number;
    i  = * ( long * ) &y;
    i  = 0x5f3759df - ( i >> 1 );
    y  = * ( float * ) &i;
    y  = y * ( f - ( x * y * y ) );
    y  = y * ( f - ( x * y * y ) );
    return number * y;
}

Mat BuildOrientationIntegralTransform(DescInfo descInfo, Mat_<float> dx, Mat_<float> dy)
{
	Size sz = dx.size();
	Mat dst(sz.height, sz.width*descInfo.nBins, CV_32F);
	float* ptr_desc = dst.ptr<float>();
	int angleBins = descInfo.applyThresholding ? descInfo.nBins - 1 : descInfo.nBins;

	double fullAngle = descInfo.signedGradient ? 360 : 180;
	float angleBase = fullAngle/double(angleBins);
	
	float* ptr_dx = dx.ptr<float>();
	float* ptr_dy = dy.ptr<float>();
	
	vector<float> sum(descInfo.nBins);
	
	for(int i = 0, index = 0; i < sz.height; i++)
	{
		sum.assign(sum.size(), 0);

		for(int j = 0; j < sz.width; j++, index++)
		{
			float shiftX = ptr_dx[index];
			float shiftY = ptr_dy[index];
			float m0 = sqrt(shiftX*shiftX+shiftY*shiftY);//FastSquareRootFloat(shiftX*shiftX + shiftY*shiftY);
			float m1 = m0;

			int bin0, bin1;
			if(descInfo.applyThresholding && m0 <= descInfo.threshold)
			{
				bin0 = angleBins;
				m0 = 1.0;
				bin1 = 0;
				m1 = 0;
			}
			else
			{
				float orientation = fastAtan2(shiftY, shiftX);
				if(orientation > fullAngle)
					orientation -= fullAngle;

				float fbin = orientation/angleBase;
				bin0 = cvFloor(fbin);
				float weight0 = 1 - (fbin - bin0);
				float weight1 = 1 - weight0;
				bin0 %= angleBins;
				bin1 = (bin0+1)%angleBins;
				//bin0 &= 7;
				//bin1 = (bin0 + 1) & 7;

				m0 *= weight0;
				m1 *= weight1;
			}

			sum[bin0] += m0;
			sum[bin1] += m1;

			int temp0 = index*descInfo.nBins;
			if(i == 0)
			{
				for(int m = 0; m < descInfo.nBins; m++)
					ptr_desc[temp0++] = sum[m];
			}
			else
			{
				int temp1 = (index - sz.width)*descInfo.nBins;
				for(int m = 0; m < descInfo.nBins; m++)
				{
					ptr_desc[temp0++] = ptr_desc[temp1++]+sum[m];
				}
			}
		}
	}
	return dst;
}

void ComputeDescriptor(Mat& integralTransform, Rect rect, DescInfo descInfo, float* desc)
{
	TIMERS.CallsComputeDescriptor++;

	const float epsilon = 0.05;

	int height = integralTransform.rows;
	int width = integralTransform.cols / descInfo.nBins;

	float* ptr_integralTransform = integralTransform.ptr<float>();

	Mat_<float> vec(1, descInfo.dim, desc, Mat::AUTO_STEP);
	float* ptr_vec = desc;
	int xOffset = rect.x;
	int yOffset = rect.y;
	int xStride = rect.width/descInfo.nxCells;
	int yStride = rect.height/descInfo.nyCells;

	for (int iX = 0, iDesc = 0; iX < descInfo.nxCells; ++iX)
	for (int iY = 0; iY < descInfo.nyCells; ++iY)
	{
		int left = xOffset + iX*xStride - 1;
		int right = std::min<int>(left + xStride + 1, width-1);
		int top = yOffset + iY*yStride - 1;
		int bottom = std::min<int>(top + yStride + 1, height-1);

		int TopLeft = (top*width+left)*descInfo.nBins;
		int TopRight = (top*width+right)*descInfo.nBins;
		int BottomLeft = (bottom*width+left)*descInfo.nBins;
		int BottomRight = (bottom*width+right)*descInfo.nBins;

		for (int i = 0; i < descInfo.nBins; ++i, ++iDesc) 
		{
			float sumTopLeft = 0, sumTopRight = 0, sumBottomLeft = 0, sumBottomRight = 0;
			if (top >= 0)
			{
				if (left >= 0)
					sumTopLeft = ptr_integralTransform[TopLeft+i];
				
				sumTopRight = ptr_integralTransform[TopRight+i];
			}
			
			if (left >= 0)
				sumBottomLeft = ptr_integralTransform[BottomLeft+i];
			
			sumBottomRight = ptr_integralTransform[BottomRight+i];

			ptr_vec[iDesc] = epsilon + sumBottomRight + sumTopLeft - sumBottomLeft - sumTopRight;
		}
	}

	//normalize(vec, vec, 1, 0, descInfo.norm);
	vec /= norm(vec, descInfo.norm);
}


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
	int frameCount;

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
		int frameCount,
		bool print = false)
		: 
		frameSizeAfterInterpolation(frameSizeAfterInterpolation), 
		ntCells(ntCells),
		tStride(tStride),
		fScale(fScale),
		frameCount(frameCount),
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

	void Update(Frame& frame, double hofCorrectionFactor)
	{
		if(hofInfo.enabled)
		{
			TIMERS.HofComputation.Start();
			hof.Update(frame.Dx*hofCorrectionFactor, frame.Dy*hofCorrectionFactor);
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
		fprintf(stderr, "Descriptor format: xnorm ynorm tnorm pts StartPTS EndPTS Xoffset Yoffset PatchWidth PatchHeight ");
		if(hogInfo.enabled)
			fprintf(stderr, "hog (dim. %d) ", hogInfo.fullDim);
		if(hofInfo.enabled)
			fprintf(stderr, "hof (dim. %d) ", hofInfo.fullDim);
		if(mbhInfo.enabled)
			fprintf(stderr, "mbhx (dim. %d) mbhy (dim. %d)", mbhInfo.fullDim, mbhInfo.fullDim);
		fprintf(stderr, "\n");
	}

	void PrintPatchDescriptorHeader(Rect rect)
	{
		int firstFrame = effectiveFrameIndices[effectiveFrameIndices.size()-ntCells*tStride];
		int lastFrame = effectiveFrameIndices.back();
		Point patchCenter(rect.x + rect.width/2, rect.y + rect.height/2);
		printf("%.2lf\t%.2lf\t%.2lf\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t", 
			double(patchCenter.x) / frameSizeAfterInterpolation.width,
			double(patchCenter.y) / frameSizeAfterInterpolation.height,
			min(double(firstFrame + lastFrame) / 2 / frameCount, 1.0),
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
			PrintFloatArray(patchDescriptor);
			
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
