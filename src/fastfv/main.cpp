#include <cstdio>
#include <string>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <cctype>
#include <opencv/cv.h>

#include "gmm_vocab.h"
#include "options.h"
#include "diag.h"
#include "opencv_utils.h"
#include "io_utils.h"
#include "../commons/log.h"
#include "spm_fv.h"

//#include "hdf5opencv.h"

#include <memory>

using namespace std;

void Print(vector<string>& comments, Mat_<float>& fv, string& outputFile)
{
	/*if(!outputFile.empty() && GetFileExtension(outputFile) == ".h5")
	{
		#if HAVE_HDF5
		static bool firstTime = true;
		unsigned int shape[] = {1, (unsigned int)fv.cols};

		for(int i = 0; i < comments.size(); i++)
		{
			hdf5opencv::hdf5save(outputFile.c_str(), comments[i].c_str(), fv.row(i), firstTime);
			firstTime = false;
		}
		#endif
	}
	else*/
	{
		FILE* out = outputFile != "" ? fopen(outputFile.c_str(), "w") : stdout;
		for(int i = 0; i < comments.size(); i++)
		{
			fprintf(out, "# %s\n", comments[i].c_str());
			PrintFloatArray(fv.ptr<float>(i), fv.cols, out);
		}
		if(out != stdout)
			fclose(out);
	}
}

int main(int argc, char* argv[])
{
	Options opts(argc, argv);
	vector<Part>& parts = opts.Parts;
	TIMERS.Total.Start();
	
	Mat_<float> features(10000, opts.UseBinary ? opts.BinaryLineSize / sizeof(float) : 500);

	if(opts.BuildGmmIndex)
	{
		TIMERS.Reading.Start();
		if(opts.UseBinary)
			ReadMatFromFileBinary(features);
		else
			ReadMatFromFile(features);
		log("read %d features", features.rows);
		TIMERS.Reading.Stop();
		
		TIMERS.Vocab.Start();
		for(int i = 0; i < parts.size(); i++)
		{
			Mat_<float> fts = features(Range::all(), Range(parts[i].FeatStart, 1+parts[i].FeatEnd)).clone();
			GmmVocab(parts[i].VocabPath, opts.GmmK, opts.Lambda).BuildAndSaveIndex(fts, opts.GmmNiter);
		}
		TIMERS.Vocab.Stop();
	}
	else
	{
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
			GmmVocab vocab(p.VocabPath, opts.GmmK, opts.Lambda);
			vocab.LoadIndex();

			indices.push_back(make_shared<Index>(vocab.mu, ass_indexParams));
			spms.push_back(SpmFisherVector(opts.Grids, vocab, p, k_nn, opts.DoSigma));
		}
		TIMERS.Vocab.Stop();
		log("Vocab loaded");

		int cnt;
		while((cnt = opts.UseBinary ? ReadBlockBinary(features) : ReadBlock(features)) > 0)
		{
			log("Read block of %d", cnt);
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
			Print(spms[i].ToString, spms[i].FV, opts.OutputFilePath);
			TIMERS.Writing.Stop();
		}
		
		for(int i = 0; i < parts.size(); i++)
		{
			spms[i].ToString.push_back("ERR");
#if HAVE_HDF5
			hdf5opencv::write_integer_attributes(opts.OutputFilePath.c_str(), spms[i].ToString, spms[i].CellStats);
#endif
		}
	}

	TIMERS.Total.Stop();
	TIMERS.Print(parts);	
}
