#include <cstdio>
#include <string>
#include <algorithm>

using namespace std;

#ifndef __GRID_PART_H__
#define __GRID_PART_H__

struct Grid
{
	int nx, ny, nt;

	char ToString[20];
	int TotalCells;
	
	int CellIndex(float x, float y, float t)
	{
		int xind = min(nx - 1, int(nx * x));
		int yind = min(ny - 1, int(ny * y));
		int tind = min(nt - 1, int(nt * t));
		return Pos(xind, yind, tind);
	}

	int Pos(int xind, int yind, int tind)
	{
		return tind + yind * nt + xind * ny * nt;
	}

	Grid(int nx = 1, int ny = 1, int nt = 1) : nx(nx), ny(ny), nt(nt)
	{
		TotalCells = nx*ny*nt;
		sprintf(ToString, "%dx%dx%d", nx, ny, nt);
	}
};

struct Part
{
	string VocabPath;
	int FeatStart;
	int FeatEnd;

	int Size;
	string ToString;

	Part(int featStart, int featEnd, string vocabPath) : FeatStart(featStart), FeatEnd(featEnd), VocabPath(vocabPath)
	{
		Size = FeatEnd - FeatStart + 1;
		
		ToString = VocabPath.substr(1 + VocabPath.find_last_of("/\\"));
		ToString = ToString.substr(0, ToString.find_first_of("."));
	}
};

#endif
