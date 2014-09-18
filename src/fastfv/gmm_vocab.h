#include <opencv/cv.h>
#include <string>

#include "opencv_utils.h"
#include <cmath>

extern "C"
{
	#include <yael/gmm.h>
}

using namespace std;
using namespace cv;

#ifndef __GMM_VOCAB_H__
#define __GMM_VOCAB_H__

struct GmmVocab
{
	string vocabPath;
	string ToString;
	int k;
	float lambda;
	
	Mat_<float> mu, sigma, w;

	GmmVocab(string vocabPath, int k, float lambda) : vocabPath(vocabPath), k(k), lambda(lambda)
	{
		ToString = vocabPath.substr(1 + vocabPath.find_last_of("/\\"));
		ToString = ToString.substr(0, ToString.find_first_of("."));
	}

	static string GetCovariancesFilePath(string vocabPath)
	{
		return vocabPath + ".cov";
	}

	static string GetWeightsFilePath(string vocabPath)
	{
		return vocabPath + ".weights";
	}

	void CheckRowMajor()
	{
		assert(mu.cols == k);
		assert(sigma.cols == k && sigma.rows == mu.rows);
		assert(w.total() == k);
	}
	
	void LoadIndex()
	{
		ReadMatFromFile(mu, vocabPath);
		ReadMatFromFile(sigma, GetCovariancesFilePath(vocabPath));
		ReadMatFromFile(w, GetWeightsFilePath(vocabPath));

		CheckRowMajor();
		
		//converting from row-major to column-major to call yael
		mu = mu.t();
		w = w.t();
		sigma = sigma.t();
		const float min_sigma = 1e-10;
		for(int i = 0; i < sigma.rows; i++)
			for(int j = 0; j < sigma.cols; j++)
				sigma(i, j) = sigma(i, j) + lambda;
				//if(sigma(i, j) < min_sigma)
				//	sigma(i,j) = min_sigma;
	}
	
	void BuildAndSaveIndex(Mat_<float> features, int niter = 50)
	{
		int d = features.cols;
		int n = features.rows;
		
		const int nthreads = 1;
		const int seed = 0;
		const int redo = 1;
		//dimensions don't match but features is already in column-major

		int flags = GMM_FLAGS_MU | GMM_FLAGS_SIGMA | GMM_FLAGS_NO_NORM;
		gmm_t* g = gmm_learn (d, n, k, niter, features.ptr<float>(), nthreads, seed, redo, flags);

		w = Mat_<float>(k, 1, g->w, Mat::AUTO_STEP);
		mu = Mat_<float>(k, d, g->mu, Mat::AUTO_STEP);
		sigma = Mat_<float>(k, d, g->sigma, Mat::AUTO_STEP);

		//converting from column-major to row-major
		w = w.t();
		mu = mu.t();
		sigma = sigma.t();

		CheckRowMajor();
		WriteMatToTextFile(vocabPath, mu);
		WriteMatToTextFile(GetCovariancesFilePath(vocabPath), sigma);
		WriteMatToTextFile(GetWeightsFilePath(vocabPath), w);
	}	
};

#endif
