#include <cstdlib>
#include <opencv/cv.h>

#include "desc_info.h"
#include "common.h"
using namespace cv;

#ifndef __INTEGRAL_TRANSFORM_H__
#define __INTEGRAL_TRANSFORM_H__

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

#endif
