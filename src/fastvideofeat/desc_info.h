#include <cstdlib>
#include <opencv/cv.h>

using namespace cv;

#ifndef __DESC_INFO_H__
#define __DESC_INFO_H__

struct DescInfo
{
    int nBins; // number of bins for vector quantization
    bool signedGradient; // 0: 180 degree; 1: 360 degree
    int norm; // 1: L1 normalization; 2: L2 normalization
    float threshold; //threshold for normalization
	bool applyThresholding; // whether thresholding or not
    int nxCells; // number of cells in x direction
    int nyCells; 
	int ntCells;
	int dim; // dimension of the descriptor
	int fullDim;
	bool enabled;

	DescInfo(int nBins, 
		bool applyThresholding, 
		int nt_cell,
		bool enabled,
		float threshold = 0.16,
		bool signedGradient = true, 
		int nxy_cell = 2):
	nBins(nBins),
	threshold(threshold),
	applyThresholding(applyThresholding),
	signedGradient(signedGradient),
	nxCells(nxy_cell),
	nyCells(nxy_cell),
	ntCells(nt_cell),
	norm(NORM_L2),
	enabled(enabled)
	{
		dim = nBins*nxCells*nyCells;
		fullDim = dim * ntCells;
	}

	void ResetPatchDescriptorBuffer(float* res)
	{
		memset(res, 0, fullDim * sizeof(float));
	}
};


#endif
