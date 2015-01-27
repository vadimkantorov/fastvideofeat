#include <vector>
#include <string>
#include <opencv/cv.h>

extern "C"
{
	#include <yael/vector.h>
	#include <yael/gmm.h>
}

#include "diag.h"

using namespace std;
using namespace cv;

#ifndef __SPM_FV_H__
#define __SPM_FV_H__

struct GmmVocab
{
	int k;
	Mat_<float> mu, sigma, w;

	GmmVocab(string vocabPath)
	{
		FILE* f = fopen(vocabPath.c_str(), "r");
		if(f == NULL)
			throw std::runtime_error("GMM vocab (" + vocabPath + ") doesn't exist or can't be opened.");
		
		gmm_t* g = gmm_read(f);
		fclose(f);
		
		k = g->k;
		int d = g->d;
		
		w = Mat_<float>(k, 1, g->w, Mat::AUTO_STEP);
		mu = Mat_<float>(k, d, g->mu, Mat::AUTO_STEP);
		sigma = Mat_<float>(k, d, g->sigma, Mat::AUTO_STEP);	
		
		float minLambda = 1e-4;
		for(int i = 0; i < sigma.rows; i++)
			for(int j = 0; j < sigma.cols; j++)
				sigma(i, j) = sigma(i, j) + minLambda;
	}
};

struct SpmFisherVector
{
	SpmFisherVector(bool enableGrids, GmmVocab& vocab, Part part, int k_nn, bool do_sigma)
		: w(vocab.w), d(part.Size), k(vocab.k), k_nn(k_nn), part(part), do_sigma(do_sigma)
	{
		mu = vocab.mu;
		sigma = vocab.sigma;

		assert((uintptr_t(mu.ptr<float>()) % 16) == 0 && mu.cols % 4 == 0);
		assert((uintptr_t(sigma.ptr<float>()) % 16) == 0 && sigma.cols % 4 == 0);
		assert(d % 4 == 0);
		
		grid = enableGrids ? Grid(1, 3, 2) : Grid(1, 1, 1);
		int fvSize = (do_sigma ? 2 : 1) * (part.Size * vocab.k);
		eff_FV = Mat_<float>::zeros(grid.TotalCells, fvSize);
		
		ComputeGammaPrecalc();
		log("%d-%d: {d: %d, k: %d, k_nn: %d, fvSize: %d, size(ToString): %d, shape(mu): %dx%d, shape(sigma): %dx%d, shape(w): %dx%d}", part.FeatStart, part.FeatEnd, d, k, k_nn, fvSize, ToString.size(), vocab.mu.rows, vocab.mu.cols, vocab.sigma.rows, vocab.sigma.cols, vocab.w.rows, vocab.w.cols);
	}

	void Update(float x, float y, float t, float* descr, int* nn)
	{
		assert((uintptr_t(descr) % 16) == 0);
		
		TIMERS.ComputeGamma.Start();
		ComputeGamma(descr, nn);
		TIMERS.ComputeGamma.Stop();
		TIMERS.UpdateFv.Start();
		UpdateFv(descr, nn, eff_FV.ptr<float>(grid.CellIndex(x, y, t)));
		TIMERS.UpdateFv.Stop();
	}
	
	void ComputeGamma(float* descr, int* nn)
	{
		float maxval = -1e30;
		for (int z = 0 ; z < k_nn ; z++) 
		{
			int j = nn[z];
			float* ptr_mu = mu.ptr<float>(j);
			float* ptr_sigma_m1 = sigma_m1.ptr<float>(j);			
			
			//below the vectorized code for:
			//float dtmp = 0;
			//for (int l = 0 ; l < d ; l++)
			//	dtmp += sqr (descr[l] - mu(j, l)) * sigma_m1(j, l);
			__m128 sum = _mm_setzero_ps();
			for(int l = 0; l < d; l += 4)
			{	
				//assert((uintptr_t)(void*)(ptr_mu + l) % 16 == 0);
				//TODO: change all loadu to load!!!!!
				__m128 descr_l = _mm_load_ps(descr + l);
				__m128 sigma_m1_l = _mm_load_ps(ptr_sigma_m1 + l);
				__m128 mu_l = _mm_load_ps(ptr_mu + l);
				
				__m128 sub = _mm_sub_ps(descr_l, mu_l);
				__m128 sqrd= _mm_mul_ps(sub, sub);
				sqrd = _mm_mul_ps(sqrd, sigma_m1_l);
				sum = _mm_add_ps(sum, sqrd);
			}
			sum = _mm_hadd_ps(sum, sum);
			sum = _mm_hadd_ps(sum, sum);
			float dtmp = _mm_cvtss_f32(sum);
			
			gamma[z] = logdetnr[j] - 0.5 * dtmp;
			if(gamma[z] > maxval) maxval = gamma[z];
		}

		float s = 0.0;
		for(int z = 0; z < k_nn; z++) {
			gamma[z] = exp(gamma[z] - maxval);
			s += gamma[z];
		}

		float is = 1.0 / s;
		for(int z = 0; z < k_nn; z++)
			gamma[z] *= is;
	}

	void UpdateFv(float* descr, int* nn, float* fv)
	{
		for(int i = 0; i < k_nn; i++)
		{
			int j = nn[i];
			float g = gamma[i];
			float* ptr_fv_mu = fv + (0) + j*d;
			float* ptr_fv_sigma = fv + (d*k) + j*d;
			float* ptr_mu = mu.ptr<float>(j);
			float* ptr_sigma = sigma.ptr<float>(j);
			
			//below the vectorized version of:
			//for(int l = 0; l < d; l++)
			//	ptr_fv[l] += g*(descr[l] - ptr_mu[l]);

			__m128 g_l = _mm_set1_ps(g);
			for(int l = 0; l < d; l += 4)
			{
				__m128 descr_l = _mm_load_ps(descr + l);
				__m128 mu_l = _mm_load_ps(ptr_mu + l);
				__m128 fv_l = _mm_load_ps(ptr_fv_mu + l);

				__m128 sub = _mm_sub_ps(descr_l, mu_l);
				__m128 res_mu = _mm_add_ps(fv_l, _mm_mul_ps(sub, g_l));
				_mm_store_ps(ptr_fv_mu + l, res_mu);
				
				if(do_sigma)
				{
					__m128 sigma_l = _mm_load_ps(ptr_sigma + l);
					__m128 sq1 = _mm_add_ps(sub, sigma_l);
					__m128 sq2 = _mm_sub_ps(sub, sigma_l);
					__m128 res_sigma = _mm_mul_ps(sq1, sq2);
					_mm_store_ps(ptr_fv_sigma + l, res_sigma);
				}
			}
		}
	}

	void ComputeGammaPrecalc()
	{
		logdetnr = fvec_new(k);
		for (int j = 0 ; j < k ; j++) {
			logdetnr[j] = -d / 2.0 * log (2 * M_PI);
			for (int l = 0 ; l < d ; l++)
				logdetnr[j] -= 0.5 * log (sigma(j, l));
			logdetnr[j] += log(w(0, j));
		}
		p = fvec_new(1*k);
		gamma = fvec_new(1*k_nn);
		pow(sigma, -1, sigma_m1);
	}

	void Done()
	{
		for(int i = 0; i < eff_FV.rows; i++)
		{
			for(int j = 0; j < k; j++)
			{
				for(int l = 0; l < d; l++)
				{
					eff_FV(i, j*d + l) *= sigma_m1(j, l);
					if(do_sigma)
						eff_FV(i, d*k + j*d + l) *= (sigma_m1(j, l) *  sigma_m1(j, l));
				}
			}
		}
		
		if(grid.nx == 1 && grid.ny == 1 && grid.nt == 1)
		{
			FV = eff_FV;
			ToString.push_back(format("#FV %s %s (%d-%d), 0-0-0", grid.ToString, part.ToString.c_str(), part.FeatStart, part.FeatEnd));
		}
		else
		{
			FV = Mat_<float>::zeros(grid.nx + grid.ny + grid.nt, eff_FV.cols);
			
			ToString.push_back(format("#FV 1x1x1 %s (%d-%d), 0-0-0", part.ToString.c_str(), part.FeatStart, part.FeatEnd));
			
			for(int i = 0; i < grid.ny; i++)
			{
				ToString.push_back(format("#FV 1x3x1 %s (%d-%d), 0-%d-0", part.ToString.c_str(), part.FeatStart, part.FeatEnd, i));
				FV.row(1 + i) = eff_FV.row(grid.Pos(0, i, 0)) + eff_FV.row(grid.Pos(0, i, 1));
			}
			
			for(int i = 0; i < grid.nt; i++)
			{
				ToString.push_back(format("#FV 1x1x2 %s (%d-%d), 0-0-%d", part.ToString.c_str(), part.FeatStart, part.FeatEnd, i));
				FV.row(4 + i) = eff_FV.row(grid.Pos(0, 0, i)) + eff_FV.row(grid.Pos(0, 1, i)) + eff_FV.row(grid.Pos(0, 2, i));
			}

			FV.row(0) = FV.row(4) + FV.row(5);
		}
	}

	float* gamma, *logdetnr, *lg;
	Mat_<float> mu, sigma, w, sigma_m1;
	Mat_<float> FV, eff_FV;
	vector<string> ToString;
	Grid grid;
	Part part;
	int d;
	int k;
	int k_nn;
	float* p;
	bool do_sigma;
};

#endif

