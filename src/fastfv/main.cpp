#include <cstdio>
#include <string>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <cctype>
#include <fstream>
#include <cstdlib>
#include <cctype>
#include <string>
#include <vector>
#include <stdexcept>
#include <opencv/cv.h>

#include "../util.h"
#include "diag.h"
#include "fisher_vector.h"
#include "common.h"

#include <memory>

using namespace std;

struct Options
{
	vector<Part> Parts;
	vector<Grid> Grids;

	int XnPos, YnPos, TnPos;
	int K_nn;
	float XnTotal, YnTotal, TnTotal;
	float Lambda;
	int FlannKdTrees;
	int FlannChecks;
	bool DoSigma;

	Options(int argc, char* argv[])
	{
		Lambda = 1e-4;
		K_nn = 5;
		DoSigma = false;
		FlannKdTrees = 4;
		FlannChecks = 32;
		XnPos = YnPos = TnPos = -1;
		XnTotal = YnTotal = TnTotal = 1.0;
		
		for(int i = 1; i < argc; )
		{
			if(strcmp(argv[i], "--lambda") == 0)
			{
				sscanf(argv[i+1], "%f", &Lambda);
				i += 2;
			}
			else if(strcmp(argv[i], "--updatesperdescriptor") == 0)
			{
				K_nn = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--flann_ntrees") == 0)
			{
				FlannKdTrees = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--flann_ncomparisons") == 0)
			{
				FlannChecks = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--enablesecondorder") == 0)
			{
				DoSigma = true;
				i++;
			}
			else if(strcmp(argv[i], "--xtot") == 0)
			{
				XnTotal = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--ytot") == 0)
			{
				YnTotal = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--ttot") == 0)
			{
				TnTotal = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--xnpos") == 0)
			{
				XnPos = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--ynpos") == 0)
			{
				YnPos = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--tnpos") == 0)
			{
				TnPos = atoi(argv[i+1]);
				i += 2;
			}
			else if(strcmp(argv[i], "--grid") == 0)
			{
				int nx, ny, nt;
				sscanf(argv[i+1], "%dx%dx%d", &nx, &ny, &nt);
				Grid g(nx, ny, nt);
				Grids.push_back(g);
				i += 2;
			}
			else if(strcmp(argv[i], "--vocab") == 0)
			{
				int a, b;
				sscanf(argv[i+1], "%d-%d", &a, &b);
				Part p(a, b, argv[i + 2]);
				Parts.push_back(p);
				i += 3;
			}
			else
				throw runtime_error("Unknown option: " + string(argv[i]));
		}
	
		
		log("# Options:");
		for(int i = 0; i < Parts.size(); i++)
		{
			log("# %d-%d\t'%s'", Parts[i].FeatStart, Parts[i].FeatEnd, Parts[i].VocabPath.c_str()); 
		}
		log("Lambda reg: %f", Lambda);
		log("K_nn: %d", K_nn);
		log("FLANN trees: %d", FlannKdTrees);
		log("FLANN checks: %d", FlannChecks);
		log("Enable second order: %s", DoSigma ? "yes" : "no");
		log("pos: {XnPos: %d, YnPos: %d, TnPos: %d}", XnPos, YnPos, TnPos);
		log("tot: {Xtot: %f, Ytot: %f, Ttot: %f}", XnTotal, YnTotal, TnTotal);
		fprintf(stderr, "Grids: [");
		for(int i = 0; i < Grids.size(); i++)
		{
			fprintf(stderr, "%s, ", Grids[i].ToString);
		}
		log("]");

		assert(XnPos != -1);
		assert(YnPos != -1);
		assert(TnPos != -1);
		assert(Grids.size() > 0);
		assert(Parts.size() > 0);
	}
};

int ReadBlock(float* features, int rows, int cols)
{
	const int MAX_LINE_LEN = 10000000;
	static char S[MAX_LINE_LEN];
	char sep[] = " \n\t";
	int cnt = 0;

	while(fgets(S, MAX_LINE_LEN - 1, stdin))
	{
		char *t = strtok(S, sep);
		if(t == NULL || strlen(t) == 0)
			break;

		for(int j = 0; t != NULL; j++) 
		{
			*(features + cnt*cols + j) = atof(t);
			t = strtok(NULL, sep);
		}
		
		cnt++;
		if(cnt == rows)
			break;
	}
	
	return cnt;
}

int main(int argc, char* argv[])
{
	Options opts(argc, argv);
	vector<Part>& parts = opts.Parts;
	TIMERS.Total.Start();
	
	Mat_<float> features(10000, 500);

	int k_nn = opts.K_nn;
					
	Mat_<int> assigned(features.rows, k_nn);
	Mat_<float> dists_dummy(features.rows, k_nn);
	
	typedef cv::flann::Index Index;
	cv::flann::KDTreeIndexParams ass_indexParams(opts.FlannKdTrees);
	cv::flann::SearchParams ass_pars(opts.FlannChecks);
			
	vector<shared_ptr<Index> > indices;
	vector<SpmFisherVector> spms;

	TIMERS.Vocab.Start();
	for(int i = 0; i < parts.size(); i++)
	{
		Part& p = parts[i];
		GmmVocab vocab(p.VocabPath, opts.Lambda);

		indices.push_back(make_shared<Index>(vocab.mu, ass_indexParams));
		spms.push_back(SpmFisherVector(opts.Grids, vocab, p, k_nn, opts.DoSigma));
	}
	TIMERS.Vocab.Stop();
	log("# Vocab loaded");

	int cnt;
	while((cnt = ReadBlock(features.ptr<float>(), features.rows, features.cols)) > 0)
	{
		log("# read block of %d", cnt);
		TIMERS.DescriptorCnt += cnt;

		for(int partInd = 0; partInd < parts.size(); partInd++)
		{
			Part& p = parts[partInd];
			shared_ptr<Index> ass_index = indices[partInd];
			SpmFisherVector& spm = spms[partInd];

			TIMERS.Copying.Start();
			Mat_<float> fts = features(Range::all(), Range(p.FeatStart, 1+p.FeatEnd)).clone();
			TIMERS.Copying.Stop();
	
			TIMERS.FLANN.Start();
			ass_index->knnSearch(fts, assigned, dists_dummy, k_nn, ass_pars);
			TIMERS.FLANN.Stop();
	
			TIMERS.Assigning.Start();
			for(int i = 0; i < cnt; i++)
			{
				float x = features(i, opts.XnPos) / opts.XnTotal;
				float y = features(i, opts.YnPos) / opts.YnTotal;
				float t = features(i, opts.TnPos) / opts.TnTotal;
				spm.Update(x, y, t,	fts.ptr<float>(i), assigned.ptr<int>(i));
			}
			TIMERS.Assigning.Stop();
			TIMERS.OpCount += cnt;
		}
	}

	for(int i = 0; i < parts.size(); i++)
	{
		TIMERS.Assigning.Start();
		spms[i].Done();
		TIMERS.Assigning.Stop();
			
		TIMERS.Writing.Start();
		Mat fv = spms[i].FV;
		for(int j = 0; j < fv.rows; j++)
		{
			printf("# %s\n", spms[i].ToString[j].c_str());
			float* v = fv.ptr<float>(j);
			for(int k = 0; k < fv.cols; k++)
				printf("%.6f ", v[k]);
			printf("\n");
		}
		TIMERS.Writing.Stop();
	}

	TIMERS.Total.Stop();
	TIMERS.Print(parts);	
}
