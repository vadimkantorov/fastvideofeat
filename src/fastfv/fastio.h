#include <cctype>
#include <cstring>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

#ifndef __FASTIO_H__
#define __FASTIO_H__

const int LINE_BUFFER_SIZE = 1000;
const int LINE_BUFFER_SIZE_STR = LINE_BUFFER_SIZE * 100;
char S[LINE_BUFFER_SIZE_STR];

template<typename T>
void ReadMatrixLineFast(char* S, T res)
{
	for(int i = 0; *S; i++)
	{
		int x = 0, pw = 1, y = 0, sign = 1;
		
		if(*S && *S == '-')
		{
			sign = -1;
			S++;
		}

		for(; *S && !isspace(*S) && *S != '.'; S++)
			y = y*10 + (*S - '0');
		
		if(*S == '.')
		{
			for(S++; *S && !isspace(*S); S++)
			{
				x = x*10 + (*S - '0');
				pw *= 10;
			}
		}

		S += strspn(S, " \t\n");
		*res = sign*(y + x / double(pw));
		res++;
	}
}

template<typename T>
void ReadMatrixFromFile(FILE* fin, vector<vector<T> >& v, int& featureSize, int& vocabSize)
{
	v.clear();
	vector<T> D;
	while(fgets(S, LINE_BUFFER_SIZE_STR, fin))
	{
		if(S[0] == '#' || strcmp(S, "") == 0)
			continue;

		if(v.empty())
			ReadMatrixLineFast(S, back_inserter(D));
		else
			ReadMatrixLineFast(S, D.begin());
		v.push_back(D);
	}
	featureSize = v.front().size();
	vocabSize = v.size();
}

template<typename T>
void ReadMatrixFromFile(string filePath, vector<vector<T> >& v, int& featureSize, int& vocabSize)
{
	FILE* file = fopen(filePath.c_str(), "r");
	ReadMatrixFromFile(file, v, featureSize, vocabSize);
	fclose(file);
}

void PrintFloatArray(float* v, int cnt, FILE* out = stdout)
{
	for(int i = 0; i < cnt; i++)
		fprintf(out, "%.8f ", v[i]);
	fprintf(out, "\n");
}

int ReadBlock(float* features, int rows, int cols)
{
	static char S[10000000];
	int cnt = 0;
	
	while(gets(S))
	{
		if(S[0] == '#' || S[0] == '\0')
			continue;

		ReadMatrixLineFast(S, features + cols*cnt);
		cnt++;

		if(cnt == rows)
			break;	
	}
	
	return cnt;
}

#endif
