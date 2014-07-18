#include <cstdio>
#include <string>

using namespace std;

#ifndef __GRID_PART_H__
#define __GRID_PART_H__

struct Grid
{
	int x, y, t;
	bool olX, olY, olT;

	double *startXs, *endXs;
	double *startYs, *endYs;
	double *startTs, *endTs;

	char ToString[20];
	int TotalCells;
	
	int CellIndex(float x_, float y_, float t_)
	{
		for(int xInd = 0; xInd < x; xInd++)
		{
			if(x_ < startXs[xInd] || x_ >= endXs[xInd])
				continue;

			for(int yInd = 0; yInd < y; yInd++)
			{
				if(y_ < startYs[yInd] || y_ >= endYs[yInd])
					continue;

				for(int tInd = 0; tInd < t; tInd++)
				{
					if(t_ >= startTs[tInd] && t_ < endTs[tInd])
					{
						return Pos(xInd, yInd, tInd);
					}
				}
			}
		}
		return Pos(0, 0, 0);
	}

	int Pos(int xInd, int yInd, int tInd)
	{
		return tInd + yInd * t + xInd * y * t;
	}

	Grid(int x, int y, int t, bool olX, bool olY, bool olT) : x(x), y(y), t(t), olX(olX), olY(olY), olT(olT)
	{
		TotalCells = x*y*t;
		startXs = new double[x];
		endXs = new double[x];
		startYs = new double[y];
		endYs = new double[y];
		startTs = new double[t];
		endTs = new double[t];
		double start, step, length;
		start = 0;
		if(olX)
		{
			step = 1. / (x + 1);
			length = step * 2;
		}
		else
			length = step = 1. / x;
		for(int j = 0; j < x; j++)
		{
			startXs[j] = start;
			endXs[j] = start + length;
			start += step;
		}
		start = 0;
		if(olY)
		{
			step = 1. / (y + 1);
			length = step * 2;
		}
		else
			length = step = 1. / y;
		for(int j = 0; j < y; j++)
		{
			startYs[j] = start;
			endYs[j] = start + length;
			start += step;
		}
		start = 0;
		if(olT)
		{
			step = 1. / (t + 1);
			length = step * 2;
		}
		else
			length = step = 1. / t;
		for(int j = 0; j < t; j++)
		{
			startTs[j] = start;
			endTs[j] = start + length;
			start += step;
		}
		//for(int i = 0; i < x*y*t; i++)
		//	fprintf(stderr, "x(%lf %lf) y(%lf %lf) t(%lf %lf)\n", startXs[i], endXs[i], startYs[i], endYs[i], startTs[i], endTs[i]);
		sprintf(ToString, "%d%c%d%c%d%c ", x, olX ? 'o' : 'x', y, olY ? 'o' : 'x', t, olT ? 'o' : 'x');
	}
};

struct Part
{
	string VocabPath;
	int FeatStart;
	int FeatEnd;

	int Size()
	{
		return FeatEnd - FeatStart + 1;
	}
};

#endif
