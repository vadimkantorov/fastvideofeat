#include <vector>
#include <string>

extern "C"
{
	#include <yael/vector.h>
	#include <yael/gmm.h>
	#include <yael/sorting.h>
}

#include "diag.h"

using namespace std;

#define DO_SIGMA false

#ifndef __SPM_FV_H__
#define __SPM_FV_H__

struct SpmFisherVector
{
	SpmFisherVector(vector<Grid> grids, GmmVocab& vocab, Part part, int k_nn, bool do_sigma)
		: w(vocab.w), d(part.Size()), k(vocab.k), k_nn(k_nn), grids(grids), do_sigma(do_sigma), descr_aligned(1, part.Size())
	{
		mu = vocab.mu;
		sigma = vocab.sigma;

		if(mu.cols % 4 != 0)
			mu = mu(Range::all(), Range(0, mu.cols / 4 * 4)).clone();
		if(sigma.cols % 4 != 0)
			sigma = sigma(Range::all(), Range(0, sigma.cols / 4 * 4)).clone();
		
		assert(d % 4 == 0);
		assert(do_sigma ? DO_SIGMA : true);

		int fvSize = (do_sigma ? 2 : 1) * (part.Size() * vocab.k);
		
		int totalCells = 0;
		char varName[200];
		for(int i = 0; i < grids.size(); i++)
		{
			Grid& g = grids[i];

			firstCell.push_back(totalCells);
			totalCells += g.TotalCells;
			for(int xInd = 0; xInd < g.x; xInd++)
			{
				for(int yInd = 0; yInd < g.y; yInd++)
				{
					for(int tInd = 0; tInd < g.t; tInd++)
					{
						sprintf(varName, "#FV %s %s (%d-%d), %d-%d-%d", g.ToString, vocab.ToString.c_str(), part.FeatStart, part.FeatEnd, xInd, yInd, tInd);
						ToString.push_back(string(varName));
					}
				}
			}
		}
		FV = Mat_<float>::zeros(totalCells, fvSize);
		CellStats = vector<int>(1 + totalCells);
		
		ComputeGammaPrecalc();

		fprintf(stderr, "d = %d, k = %d, k_nn = %d, fvSize = %d, size(ToString) = %d, size(grids) = %d, size(firstCell) = %d, shape(mu) = %dx%d, shape(sigma) = %dx%d, shape(w) = %dx%d\n", d, k, k_nn, fvSize, ToString.size(), grids.size(), firstCell.size(), vocab.mu.rows, vocab.mu.cols, vocab.sigma.rows, vocab.sigma.cols, vocab.w.rows, vocab.w.cols);
	}

	void Update(float x, float y, float t, float* descr, int* nn)
	{
		if(!((uintptr_t(descr) & 15) == 0))
		{
			memcpy(descr_aligned.ptr<float>(), descr, sizeof(float)*d);
			descr = descr_aligned.ptr<float>();
			fprintf(stderr, "Not aligned. Replacing by %lu\n", (unsigned long)descr_aligned.ptr<float>());
		}
	
		assert((uintptr_t(descr) & 15) == 0);
		
		TIMERS.ComputeGamma.Start();
		ComputeGamma(descr, nn);
		TIMERS.ComputeGamma.Stop();
		TIMERS.UpdateFv.Start();
		for(int i = 0; i < grids.size(); i++)
		{
			int ind = firstCell[i] + grids[i].CellIndex(x, y, t);
			UpdateFv(descr, nn, FV.ptr<float>(ind));
			if(ind >= 0 && ind < (CellStats.size() - 1))
				CellStats[ind]++;
			else
				CellStats[CellStats.size()-1]++;
		}
		TIMERS.UpdateFv.Stop();
	}
	
	void Assign(Mat_<float>& fts, Mat_<int>& assigned)
	{
		float* p = fvec_new(fts.rows*k);
		gmm_compute_p(fts.rows, fts.ptr<float>(), p);
		for(int i = 0; i < fts.rows; i++)
		{
			fvec_k_max(&p[i*k], k, assigned.ptr<int>(i), k_nn);
		}
		free(p);
	}
		
	void gmm_compute_p (int n, const float * v, float * p)
	{
		for (int i = 0 ; i < n ; i++) 
		{
			float maxval = -1e30;
			for (int j = 0 ; j < k ; j++) {
				double dtmp = 0;
				for (int l = 0 ; l < d ; l++) {
					dtmp += sqr (v[i * d + l] - mu(j, l)) * sigma_m1(j, l);
				}
				p[i * k + j] = logdetnr[j] - 0.5 * dtmp;
				if(p[i*k + j] > maxval) maxval = p[i*k + j];
			}

			float s = 0.0;
			for(int j = 0; j < k; j++) {
				p[i*k + j] = exp(p[i*k + j] - maxval);
				s += p[i*k + j];
			}

			float is = 1.0 / s;
			for(int l = 0; l < k; l++)
				p[i*k + l] *= is;
		}
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

	void ComputeGammaOld(float* descr, int* nn)
	{
		gmm_compute_p(1, descr, p);
		
		for(int z = 0; z < k_nn; z++)
		{
			int j = nn[z];
			gamma[z] = p[j];
		}
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
				
#ifdef DO_SIGMA
				if(do_sigma)
				{
					__m128 sigma_l = _mm_load_ps(ptr_sigma + l);
					__m128 sq1 = _mm_add_ps(sub, sigma_l);
					__m128 sq2 = _mm_sub_ps(sub, sigma_l);
					__m128 res_sigma = _mm_mul_ps(sq1, sq2);
					_mm_store_ps(ptr_fv_sigma + l, res_sigma);
				}
#endif
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
		for(int i = 0; i < FV.rows; i++)
			for(int j = 0; j < k; j++)
				for(int l = 0; l < d; l++)
				{
					FV(i, j*d + l) *= sigma_m1(j, l);
#ifdef DO_SIGMA
					if(do_sigma)
						FV(i, d*k + j*d + l) *= (sigma_m1(j, l) *  sigma_m1(j, l));
#endif
				}
	}

	float* gamma, *logdetnr, *lg;
	Mat_<float> mu, sigma, w,sigma_m1;
	Mat_<float> descr_aligned;
	Mat_<float> FV;
	vector<string> ToString;
	vector<int> firstCell;
	vector<Grid> grids;
	int d;
	int k;
	int k_nn;
	float* p;
	bool do_sigma;
	vector<int> CellStats;
};

#endif

