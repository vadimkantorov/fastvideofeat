#include <opencv/cv.h>

#include "../Commons/motion_vector_file_utils.h"

using namespace cv;

#ifndef __VIS_H__
#define __VIS_H__

bool IsGoodMotionVector(MotionVector& mv)
{
	double len = sqrt(mv.Dx*mv.Dx + mv.Dy*mv.Dy);
	return len < 50;
}

void drawArrow(cv::Mat& img, cv::Point pStart, cv::Point pEnd, int len, int alpha,
    cv::Scalar& color, int thickness = 1, int lineType = CV_AA)
{    
	const double PI = 3.1415926;    
	double angle = atan2((double)(pStart.y - pEnd.y), (double)(pStart.x - pEnd.x));  
	line(img, pStart, pEnd, color, thickness, lineType);   
	if(len != 0)
	{
		Point arrow;    
		arrow.x = pEnd.x + len * cos(angle + PI * alpha / 180);     
		arrow.y = pEnd.y + len * sin(angle + PI * alpha / 180);  
		line(img, pEnd, arrow, color, thickness, lineType);   
		arrow.x = pEnd.x + len * cos(angle - PI * alpha / 180);     
		arrow.y = pEnd.y + len * sin(angle - PI * alpha / 180);    
		line(img, pEnd, arrow, color, thickness, lineType);
	}
}

double degrees(double x)
{
	return x;
}

MotionVector NormalizeMotionVector(MotionVector mv, double len = 2)
{
	MotionVector res = mv;
	double coeff = len/sqrt(double(mv.Dx*mv.Dx + mv.Dy*mv.Dy));
	res.Dx = coeff*mv.Dx;
	res.Dy = coeff*mv.Dy;
	return res;
}

void DrawArrows(Mat& img, Mat_<float>& dx, Mat_<float>&dy, Mat_<bool>& missing, Scalar color = CV_RGB(255,0,0), bool invert = false)
{
	for(int i = 0; i < dx.rows; i++)
	{
		for(int j = 0; j < dx.cols; j++)
		{
			MotionVector mv;
			mv.Dx = dx(i, j);
			mv.Dy = dy(i, j);
			mv.X = double(j) / dx.cols * img.cols;
			mv.Y = double(i) / dx.rows * img.rows;
			//if(IsGoodMotionVector(mv))
			{
				//mv = NormalizeMotionVector(mv, 10);
				int sign = invert ? -1 : +1;
				Point start(mv.X, mv.Y);
				Point end(mv.X + sign*mv.Dx, mv.Y + sign*mv.Dy);
			
				drawArrow(img, start, end, 0, degrees(20.0), color);
				img.at<cv::Vec3b>(start) = Vec3b(0,255,255);
			}
		}
	}
}

#endif