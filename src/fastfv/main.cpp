#include <cstdio>
#include <string>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <cctype>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <opencv/cv.h>

#include "../util.h"
#include "diag.h"
#include "fisher_vector.h"
#include "common.h"

extern "C"
{
	#include <yael/gmm.h>
	#include <yael/nn.h>
}

using namespace cv;
using namespace cv::flann; 
using namespace std;

struct Options
{
	vector<Part> Parts;

	int XnPos, YnPos, TnPos;
	int K_nn;
	int FlannKdTrees, FlannChecks;
	bool DoSigma, EnableGrids;

	Options(int argc, char* argv[])
	{
		K_nn = 5;
		EnableGrids = DoSigma = false;
		FlannKdTrees = FlannChecks = -1;
		XnPos = YnPos = TnPos = -1;
		
		for(int i = 1; i < argc; i++)
		{
			if(strcmp(argv[i], "--knn") == 0)
			{
				K_nn = atoi(argv[i+1]);
				i++;
			}
			else if(strcmp(argv[i], "--enableflann") == 0)
			{
				FlannKdTrees = 4;
				FlannChecks = 32;
				
				if(i + 2 < argc && sscanf(argv[i + 1], "%d", &FlannKdTrees) == 1 && sscanf(argv[i + 2], "%d", &FlannChecks) == 1)
				{
					i += 2;
				}
			}
			else if(strcmp(argv[i], "--enablesecondorder") == 0)
			{
				DoSigma = true;
			}
			else if(strcmp(argv[i], "--xpos") == 0)
			{
				XnPos = atoi(argv[i + 1]);
				i++;
			}
			else if(strcmp(argv[i], "--ypos") == 0)
			{
				YnPos = atoi(argv[i + 1]);
				i++;
			}
			else if(strcmp(argv[i], "--tpos") == 0)
			{
				TnPos = atoi(argv[i + 1]);
				i++;
			}
			else if(strcmp(argv[i], "--enablespatiotemporalgrids") == 0)
			{
				EnableGrids = true;
			}
			else if(strcmp(argv[i], "--vocab") == 0)
			{
				int a, b;
				sscanf(argv[i+1], "%d-%d", &a, &b);
				Part p(a, b, argv[i + 2]);
				Parts.push_back(p);
				i += 2;
			}
			else
				throw runtime_error("Unknown option: " + string(argv[i]));
		}
	
		
		log("# Options:");
		for(int i = 0; i < Parts.size(); i++)
		{
			log("# %d-%d\t'%s'", Parts[i].FeatStart, Parts[i].FeatEnd, Parts[i].VocabPath.c_str()); 
		}
		log("K_nn: %d", K_nn);
		log("pos: {XnPos: %d, YnPos: %d, TnPos: %d}", XnPos, YnPos, TnPos);
		log("FLANN: {trees: %d, checks: %d}", FlannKdTrees, FlannChecks);
		log("Enable second order: %s", DoSigma ? "yes" : "no");
		log("Enable spatio-temporal grids (1x1x1, 1x3x1, 1x1x2): %s", EnableGrids ? "yes" : "no");

		assert(XnPos != -1);
		assert(YnPos != -1);
		assert(TnPos != -1);
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
	TIMERS.Total.Start();
	Options opts(argc, argv);
	setNumThreads(1);
	vector<Part>& parts = opts.Parts;
	
	TIMERS.Vocab.Start();
	KDTreeIndexParams ass_indexParams(opts.FlannKdTrees != -1 ? opts.FlannKdTrees : 4);
	SearchParams ass_pars(opts.FlannChecks != -1 ? opts.FlannChecks : 32);
	
	vector<GmmVocab> vocabs;
	vector<shared_ptr<Index> > indices;
	vector<SpmFisherVector> spms;
	
	for(int i = 0; i < parts.size(); i++)
	{
		Part& p = parts[i];
		GmmVocab vocab(p.VocabPath);
		vocabs.push_back(vocab);
		indices.push_back(make_shared<Index>(vocab.mu, ass_indexParams));
		spms.push_back(SpmFisherVector(opts.EnableGrids, vocab, p, opts.K_nn, opts.DoSigma));
	}
	TIMERS.Vocab.Stop();
	log("# Vocab loaded");
	
	Mat_<float> features(1000, 500);
	Mat_<int> assigned(features.rows, opts.K_nn);
	Mat_<float> dists_dummy(features.rows, opts.K_nn);

	int cnt;
	while((cnt = ReadBlock(features.ptr<float>(), features.rows, features.cols)) > 0)
	{
		log("# read block of %d", cnt);
		TIMERS.DescriptorCnt += cnt;

		for(int partInd = 0; partInd < parts.size(); partInd++)
		{
			TIMERS.Copying.Start();
			Mat_<float> fts = features(Range::all(), Range(parts[partInd].FeatStart, 1 + parts[partInd].FeatEnd)).clone();
			TIMERS.Copying.Stop();
	
			TIMERS.FLANN.Start();
			if(opts.FlannKdTrees != -1 && opts.FlannChecks != -1)
			{
				shared_ptr<Index> ass_index = indices[partInd];
				ass_index->knnSearch(fts, assigned, dists_dummy, opts.K_nn, ass_pars);
			}
			else
			{
				Mat mu = vocabs[partInd].mu;
				knn_full (2, fts.rows, mu.rows, mu.cols, opts.K_nn, mu.ptr<float>(), fts.ptr<float>(), NULL, assigned.ptr<int>(), dists_dummy.ptr<float>());
			}
			TIMERS.FLANN.Stop();
	
			TIMERS.Assigning.Start();
			for(int i = 0; i < cnt; i++)
			{
				spms[partInd].Update(features(i, opts.XnPos), features(i, opts.YnPos), features(i, opts.TnPos), fts.ptr<float>(i), assigned.ptr<int>(i));
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
