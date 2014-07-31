#include <algorithm>
#include <iterator>
#include <string>

#include <opencv/cv.h>
#include "fastio.h"
#include "io_utils.h"

#include <vector>

using namespace std;
using namespace cv;

#ifndef __OPENCV_UTILS_H__
#define __OPENCV_UTILS_H__

template<typename T>
void FillMat(vector<vector<T> >& v, Mat_<T>& res)
{
	int rows = v.size();
	int cols = v.front().size();
	res.create(rows, cols);
	for(int i = 0; i < rows; i++)
		memcpy(res.ptr(i), &v[i][0], res.cols*res.elemSize());
}

template<typename T>
void ReadMatFromFile(Mat_<T>& res, string filePath)
{
	int cols, rows;
	vector<vector<float> > v;
	ReadMatrixFromFile(filePath, v, cols, rows);
	FillMat(v, res);
}

template<typename T>
void ReadMatFromFile(Mat_<T>& res, FILE* in = stdin)
{
	int cols, rows;
	vector<vector<float> > v;
	ReadMatrixFromFile(in, v, cols, rows);
	FillMat(v, res);
}

void WriteMatToTextFile(string filePath, Mat_<float>& res)
{
	FILE* out = fopen(filePath.c_str(), "w");
	assert(out != NULL);
	for(int i = 0; i < res.rows; i++)
	{
		PrintFloatArray(res.ptr<float>(i), res.cols, out);
	}
	fclose(out);
}

int ReadBlock(Mat_<float>& res)
{
	return ReadBlock(res.ptr<float>(), res.rows, res.cols);
}

int ReadBlockBinary(Mat_<float>& res, FILE* stream = stdin)
{
	int lineSize = res.cols;
	int k = 0;
	while(k < res.rows)
	{
		if(fread(res.ptr<float>(k), sizeof(float), lineSize, stream) != lineSize)
			break;
		
		k++;
	}
	return k;
}

void ReadMatFromFileBinary(Mat_<float>& res, FILE* stream = stdin)
{
	int lineSize = res.cols;
	vector<vector<float> > v;
	vector<float> vv(lineSize);
	while(true)
	{
		if(fread(&vv[0], sizeof(float), lineSize, stream) != lineSize)
			break;
		v.push_back(vv);
	}
	FillMat(v, res);
}

#endif
