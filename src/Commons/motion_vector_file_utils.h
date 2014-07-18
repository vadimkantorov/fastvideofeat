#include <string>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>

using namespace std;

#ifndef __READ_MOTION_VECTOR_FILE_H__
#define __READ_MOTION_VECTOR_FILE_H__

const int NO_MV = -10000;

struct MotionVector
{
	int X,Y;
	float Dx,Dy;

	int Mx, My;
	char TypeCode, SegmCode;

	bool NoMotionVector()
	{
		return (Dx == NO_MV && Dy == NO_MV) || (Dx == -NO_MV && Dy == -NO_MV);
	}

	bool IsIntra()
	{
		return TypeCode == 'P' || TypeCode == 'A' || TypeCode == 'i' || TypeCode == 'I';
	}
};

typedef pair<int, vector<MotionVector> > FlowPoints;

struct MotionVectorFileWriter
{
	FILE* motionVectorsFile;

	MotionVectorFileWriter(string path = "")
	{
		if(path != "")
			Open(path);
	}

	void Open(string motionVectorsPath)
	{
		motionVectorsFile = fopen(motionVectorsPath.c_str(), "w");
		fputs("FrameIndex\tX\tY\tDx\tDy\tMacroBlockTopLeftCornerX\tMacroBlockTopLeftCornerY\tTypeSegm\n", motionVectorsFile);
	}

	void Write(int index, int sx, int sy, double dx, double dy, int mx = -1, int my = -1, char typeCode = '_', char segmCode = '_')
	{
		fprintf(motionVectorsFile, "%d\t%d\t%d\t%.2f\t%.2f\t%d\t%d\t%c%c\n", index, sx, sy, dx, dy, mx, my, typeCode, segmCode);
	}

	~MotionVectorFileWriter()
	{
		fclose(motionVectorsFile);
	}
};

struct MotionVectorFileReader2
{
	FILE* motionVectorsFile;
	char curLine[200];

	void Open(string motionVectorsPath)
	{
		motionVectorsFile = fopen(motionVectorsPath.c_str(), "r");
		fgets(curLine,100,motionVectorsFile);
		fgets(curLine,100,motionVectorsFile);
	}

	MotionVectorFileReader2(string motionVectorsPath = "")
	{
		motionVectorsFile = NULL;
		if(motionVectorsPath != "")
			Open(motionVectorsPath);
	}

	FlowPoints ReadFlowPoints()
	{
		vector<MotionVector> pts;
		if(strcmp(curLine,"") == 0)
			return make_pair(-1, pts);
	
		int curFrameIndex, frameIndex;
		MotionVector mv;

		sscanf(curLine, "%d %d %d %f %f %d %d %c%c", &curFrameIndex, &mv.X, &mv.Y, &mv.Dx, &mv.Dy, &mv.Mx, &mv.My, &mv.TypeCode, &mv.SegmCode);
		pts.push_back(mv);
		while(true)
		{
			strcpy(curLine, "");
			if(fgets(curLine, 100, motionVectorsFile) == NULL)
				break;
			if(strcmp(curLine, "") == 0)
				break;
			sscanf(curLine, "%d %d %d %f %f %d %d %c%c", &frameIndex, &mv.X, &mv.Y, &mv.Dx, &mv.Dy, &mv.Mx, &mv.My, &mv.TypeCode, &mv.SegmCode);
			if(frameIndex != curFrameIndex)
				break;
			pts.push_back(mv);
		}

		return make_pair(curFrameIndex, pts);
	}

	~MotionVectorFileReader2()
	{
		if(motionVectorsFile != NULL)
			fclose(motionVectorsFile);
	}
};


#endif
