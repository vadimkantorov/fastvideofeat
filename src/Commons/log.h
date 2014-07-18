#include <cstdio>
#include <cstdlib>
#include <opencv/cv.h>

#ifndef __LOG_H__
#define __LOG_H__

static bool log_enabled = true;

void log_enable()
{
	log_enabled = true;
}

void log_disable()
{
	log_enabled = false;
}

void log(FILE* out, const char* fmt, ...)
{
	if(log_enabled)
	{
		va_list argp;
		va_start(argp, fmt);
		vfprintf(out, fmt, argp);
		va_end(argp);
		fprintf(out, "\n");
		fflush(out);
	}
}

void log(const char* fmt, ...)
{
	FILE* out = stderr;
	if(log_enabled)
	{
		va_list argp;
		va_start(argp, fmt);
		vfprintf(out, fmt, argp);
		va_end(argp);
		fprintf(out, "\n");
		fflush(out);
	}
}

void logmat(cv::Mat& m, const char* name = "")
{
	int type = m.type();
	uchar depth = type & CV_MAT_DEPTH_MASK;
	uchar chans = 1 + (type >> CV_CN_SHIFT);
	string r = "CV_";
	switch ( depth ) {
		case CV_8U:  r += "8U"; break;
		case CV_8S:  r += "8S"; break;
		case CV_16U: r += "16U"; break;
		case CV_16S: r += "16S"; break;
		case CV_32S: r += "32S"; break;
		case CV_32F: r += "32F"; break;
		case CV_64F: r += "64F"; break;
		default:     r += "User"; break;
	}

	r += "C";
	r += (chans+'0');

	log("%s: %dx%d %s", strlen(name) == 0 ? "<no name>" : name, m.rows, m.cols, r.c_str());
}

#endif
