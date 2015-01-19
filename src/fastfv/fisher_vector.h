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
	float lambda;
	int k;
	Mat_<float> mu, sigma, w;

	GmmVocab(string vocabPath, float lambda) : lambda(lambda)
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
		
		const float min_sigma = 1e-10;
		for(int i = 0; i < sigma.rows; i++)
			for(int j = 0; j < sigma.cols; j++)
				sigma(i, j) = sigma(i, j) + lambda;
	}
};

struct SpmFisherVector
{
	static int LCM(int a_, int b_)
	{
		int a = a_, b = b_, c;
		while((c = a % b) != 0)
		{
			a = b;
			b = c;
		}
		int gcd = b;
		return a / gcd * b;
	}
	
	SpmFisherVector(vector<Grid> grids, GmmVocab& vocab, Part part, int k_nn, bool do_sigma)
		: w(vocab.w), d(part.Size), k(vocab.k), k_nn(k_nn), grids(grids), do_sigma(do_sigma), descr_aligned(1, part.Size)
	{
		mu = vocab.mu;
		sigma = vocab.sigma;

		if(mu.cols % 4 != 0)
			mu = mu(Range::all(), Range(0, mu.cols / 4 * 4)).clone();
		if(sigma.cols % 4 != 0)
			sigma = sigma(Range::all(), Range(0, sigma.cols / 4 * 4)).clone();
		
		assert(d % 4 == 0);
		int fvSize = (do_sigma ? 2 : 1) * (part.Size * vocab.k);
		
		char varName[200];
		int totalCells = 0;
		for(int i = 0; i < grids.size(); i++)
		{
			Grid& g = grids[i];
			eff = Grid(LCM(eff.nx, g.nx), LCM(eff.ny, g.ny), LCM(eff.nt, g.nt));
			
			firstCell.push_back(totalCells);
			totalCells += g.TotalCells;

			for(int xInd = 0; xInd < g.nx; xInd++)
			{
				for(int yInd = 0; yInd < g.ny; yInd++)
				{
					for(int tInd = 0; tInd < g.nt; tInd++)
					{
						sprintf(varName, "#FV %s %s (%d-%d), %d-%d-%d", g.ToString, part.ToString.c_str(), part.FeatStart, part.FeatEnd, xInd, yInd, tInd);
						ToString.push_back(string(varName));
					}
				}
			}
		}
		
		eff_FV = Mat_<float>::zeros(eff.TotalCells, fvSize);
		FV = Mat_<float>::zeros(totalCells, fvSize);
		
		ComputeGammaPrecalc();
		log("%d-%d: {d: %d, k: %d, k_nn: %d, fvSize: %d, size(ToString): %d, size(grids): %d, shape(mu): %dx%d, shape(sigma): %dx%d, shape(w): %dx%d}", part.FeatStart, part.FeatEnd, d, k, k_nn, fvSize, ToString.size(), grids.size(), vocab.mu.rows, vocab.mu.cols, vocab.sigma.rows, vocab.sigma.cols, vocab.w.rows, vocab.w.cols);
	}

	void Update(float x, float y, float t, float* descr, int* nn)
	{
		if(!((uintptr_t(descr) & 15) == 0))
		{
			descr = descr_aligned.ptr<float>();
			log("# Not aligned. Replacing by %lu", (unsigned long)descr_aligned.ptr<float>());
		}
	
		assert((uintptr_t(descr) & 15) == 0);
		
		TIMERS.ComputeGamma.Start();
		ComputeGamma(descr, nn);
		TIMERS.ComputeGamma.Stop();
		TIMERS.UpdateFv.Start();
		UpdateFv(descr, nn, eff_FV.ptr<float>(eff.CellIndex(x, y, t)));
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

	double static inline sqr(double x)
	{
		return x * x;
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
	
		for(int i = 0; i < grids.size(); i++)
		{
			Grid& g = grids[i];
			for(int xind = 0; xind < g.nx; xind++)
			{
				for(int yind = 0; yind < g.ny; yind++)
				{
					for(int tind = 0; tind < g.nt; tind++)
					{
						Mat target = FV.row(firstCell[i] + g.Pos(xind, yind, tind));
						int dx = eff.nx / g.nx;
						int dy = eff.ny / g.ny;
						int dt = eff.nt / g.nt;

						for(int eff_xind = xind * dx; eff_xind < (1 + xind) * dx; eff_xind++)
						{
							
							for(int eff_yind = yind * dy; eff_yind < (1 + yind) * dy; eff_yind++)
							{
								for(int eff_tind = tind * dt; eff_tind < (1 + tind) * dt; eff_tind++)
								{
									target += eff_FV.row(eff.Pos(eff_xind, eff_yind, eff_tind));
								}
							}
						}
					}
				}
			}
		}
	}

	float* gamma, *logdetnr, *lg;
	Mat_<float> mu, sigma, w,sigma_m1;
	Mat_<float> descr_aligned;
	Mat_<float> FV, eff_FV;
	vector<string> ToString;
	vector<Grid> grids;
	Grid eff;
	int d;
	int k;
	int k_nn;
	float* p;
	bool do_sigma;
	vector<int> firstCell;
};

#endif

