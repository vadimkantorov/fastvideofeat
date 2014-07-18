#include <string>
#include <cstdio>
#include <algorithm>
#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>

#include "../Commons/motion_vector_file_utils.h"
#include "../Commons/log.h"
#include "frame_reader.h"
#include "histogram_buffer.h"
#include "desc_info.h"
#include "options.h"
#include "diag.h"
#include "vis.h"

#include <iterator>
#include <vector>

using namespace std;
using namespace cv;

static void MyWarpPerspective(Mat& prev_src, Mat& src, Mat& dst, Mat& M0, int flags = INTER_LINEAR,
                             int borderType = BORDER_CONSTANT, const Scalar& borderValue = Scalar())
{
    int width = src.cols;
    int height = src.rows;
    dst.create( height, width, CV_8UC1 );

    Mat mask = Mat::zeros(height, width, CV_8UC1);
    const int margin = 5;

    const int BLOCK_SZ = 32;
    short XY[BLOCK_SZ*BLOCK_SZ*2], A[BLOCK_SZ*BLOCK_SZ];

    int interpolation = flags & INTER_MAX;
    if( interpolation == INTER_AREA )
        interpolation = INTER_LINEAR;

    double M[9];
    Mat matM(3, 3, CV_64F, M);
    M0.convertTo(matM, matM.type());
    if( !(flags & WARP_INVERSE_MAP) )
		         invert(matM, matM);

    int x, y, x1, y1;

    int bh0 = std::min(BLOCK_SZ/2, height);
    int bw0 = std::min(BLOCK_SZ*BLOCK_SZ/bh0, width);
    bh0 = std::min(BLOCK_SZ*BLOCK_SZ/bw0, height);

    for( y = 0; y < height; y += bh0 ) {
    for( x = 0; x < width; x += bw0 ) {
        int bw = std::min( bw0, width - x);
        int bh = std::min( bh0, height - y);

        Mat _XY(bh, bw, CV_16SC2, XY);
        Mat matA;
        Mat dpart(dst, Rect(x, y, bw, bh));

        for( y1 = 0; y1 < bh; y1++ ) {

            short* xy = XY + y1*bw*2;
            double X0 = M[0]*x + M[1]*(y + y1) + M[2];
            double Y0 = M[3]*x + M[4]*(y + y1) + M[5];
            double W0 = M[6]*x + M[7]*(y + y1) + M[8];
            short* alpha = A + y1*bw;

            for( x1 = 0; x1 < bw; x1++ ) {

                double W = W0 + M[6]*x1;
                W = W ? INTER_TAB_SIZE/W : 0;
                double fX = std::max((double)INT_MIN, std::min((double)INT_MAX, (X0 + M[0]*x1)*W));
                double fY = std::max((double)INT_MIN, std::min((double)INT_MAX, (Y0 + M[3]*x1)*W));
				
                double _X = fX/double(INTER_TAB_SIZE);
                double _Y = fY/double(INTER_TAB_SIZE);

                if( _X > margin && _X < width-1-margin && _Y > margin && _Y < height-1-margin )
                    mask.at<uchar>(y+y1, x+x1) = 1;

                int X = saturate_cast<int>(fX);
                int Y = saturate_cast<int>(fY);

                xy[x1*2] = saturate_cast<short>(X >> INTER_BITS);
                xy[x1*2+1] = saturate_cast<short>(Y >> INTER_BITS);
                alpha[x1] = (short)((Y & (INTER_TAB_SIZE-1))*INTER_TAB_SIZE + (X & (INTER_TAB_SIZE-1)));
            }
        }

        Mat _matA(bh, bw, CV_16U, A);
        remap( src, dpart, _XY, _matA, interpolation, borderType, borderValue );
    }
    }

    for( y = 0; y < height; y++ ) {
        const uchar* m = mask.ptr<uchar>(y);
        const uchar* s = prev_src.ptr<uchar>(y);
        uchar* d = dst.ptr<uchar>(y);
        for( x = 0; x < width; x++ ) {
            if(m[x] == 0)
                d[x] = s[x];
        }
    }
}


int main(int argc, char* argv[])
{
	Options opts(argc, argv);

	const int nt_cell = 3;
	const int tStride = 5;
	vector<Size> patchSizes;
	patchSizes.push_back(Size(32, 32));
	patchSizes.push_back(Size(48, 48));

	DescInfo hofInfo(8+1, true, nt_cell, opts.HofEnabled);
	DescInfo mbhInfo(8, false, nt_cell, opts.MbhEnabled);
	DescInfo hogInfo(8, false, nt_cell, opts.HogEnabled);

	TIMERS.Reading.Start();
	FrameReader rdr(opts.VideoPath, hogInfo.enabled);
	TIMERS.Reading.Stop();

	Size frameSizeAfterInterpolation = 
		opts.Interpolation
			? opts.UseFarnebackFlow 
				? Size(rdr.OriginalFrameSize.width / opts.FlowResample, rdr.OriginalFrameSize.height / opts.FlowResample) 
				: Size(2*rdr.DownsampledFrameSize.width - 1, 2*rdr.DownsampledFrameSize.height - 1)
			: rdr.DownsampledFrameSize;
	int cellSize = rdr.OriginalFrameSize.width / frameSizeAfterInterpolation.width;
	double fscale = opts.UseFarnebackFlow ? 1.0 : 1 / 8.0;

	log("Frame count:\t%d", rdr.FrameCount);
	log("Original frame size:\t%dx%d", rdr.OriginalFrameSize.width, rdr.OriginalFrameSize.height);
	log("Downsampled:\t%dx%d", rdr.DownsampledFrameSize.width, rdr.DownsampledFrameSize.height);
	log("After interpolation:\t%dx%d", frameSizeAfterInterpolation.width, frameSizeAfterInterpolation.height);
	log("CellSize:\t%d", cellSize);

	HofMbhBuffer buffer(hogInfo, hofInfo, mbhInfo, nt_cell, tStride, frameSizeAfterInterpolation, fscale, opts.PrintDescriptors);
 	buffer.PrintFileHeader();

	TIMERS.Everything.Start();
	Mat prevRawImageGray, currentRawImageGray;
	while(true)
	{
		Frame frame = rdr.Read();
		if(frame.PTS == -1)
			break;

		log("#read frame pts=%d", frame.PTS);

		if(opts.GoodPts.empty() || count(opts.GoodPts.begin(), opts.GoodPts.end(), frame.PTS) == 1)
		{
			if(opts.DumpFrames)
			{
				log("#dumping raw frame pts=%d", frame.PTS);
				stringstream ss;
				ss << opts.OutputDir << "/" << frame.PTS << ".jpg";
				imwrite(ss.str(), frame.RawImage);
			}
			/*if(!frame.RawImage.empty())
			{
				Mat img = frame.RawImage.clone();
				DrawArrows(img, frame.Dx, frame.Dy, frame.Missing);
				imshow("", img);
				waitKey();
			}*/

			TIMERS.DescriptorComputation.Start();
			if(opts.UseFarnebackFlow)
			{
				cvtColor(frame.RawImage, currentRawImageGray, CV_BGR2GRAY);
				Mat prev;
				swap(prevRawImageGray, prev);
				prevRawImageGray = currentRawImageGray.clone();

				if(prev.empty())
				{
					log("#skipping");
					TIMERS.SkippedFrames++;
					continue;
				}

				Mat flow_mat;
				cv::calcOpticalFlowFarneback( prev, currentRawImageGray, flow_mat,
							sqrt(2.0)/2.0, 5, 10, 2, 7, 1.5, cv::OPTFLOW_FARNEBACK_GAUSSIAN );
				Mat splitted[2];
				split(flow_mat, splitted);
				frame.Dx = splitted[0];
				frame.Dy = splitted[1];
				frame.NoMotionVectors = false;
				frame.Interpolate(frameSizeAfterInterpolation, fscale);
			}
			else if(opts.UseWarping)
			{
				cvtColor(frame.RawImage, currentRawImageGray, CV_BGR2GRAY);
				Mat prev;
				swap(prevRawImageGray, prev);
				prevRawImageGray = currentRawImageGray.clone();

				if(frame.NoMotionVectors || (hogInfo.enabled && frame.RawImage.empty()))
				{
					log("#skipping");
					TIMERS.SkippedFrames++;
					continue;
				}
				
				vector<Point2f> prev_pts_all, pts_all;
				for(int i = 0; i < frame.Dx.rows; i++)
				{
					for(int j = 0; j < frame.Dx.cols; j++)
					{
						Point2f u(16*i + 8, 16*j + 8);
						Point2f v(u.x + frame.Dx(i, j), u.y + frame.Dy(i, j));
						prev_pts_all.push_back(u);
						pts_all.push_back(v);
					}
				}

				if(!prevRawImageGray.empty())
				{
					Mat_<double> H = findHomography(prev_pts_all, pts_all, RANSAC);
					//Mat H_inv = H.inv();
					//logmat(H);
					//PrintDoubleArray(H);
					//Mat grey_warp = Mat::zeros(currentRawImageGray.size(), CV_8UC1);
					//MyWarpPerspective(prev, currentRawImageGray, grey_warp, H_inv);
					frame.WarpDx.create(frame.Dx.size());
					frame.WarpDy.create(frame.Dx.size());
					for(int i = 0; i < frame.Dx.rows; i++)
					{
						for(int j = 0; j < frame.Dx.cols; j++)
						{
							if(frame.Dx(i, j)*frame.Dx(i, j) + frame.Dy(i, j)*frame.Dy(i, j) > 1.0)
							{
								Point2f u(16*i + 8, 16*j + 8);
								Point2f v(u.x + frame.Dx(i, j), u.y + frame.Dy(i, j));
								
								double norm = 1.0 / (H(2, 0) * u.x + H(2, 1) * u.y + H(2, 2));
								frame.WarpDx(i, j) = v.x - (H(0, 0) * u.x + H(0, 1) * u.y + H(0, 2))*norm;
								frame.WarpDy(i, j) = v.y - (H(1, 0) * u.x + H(1, 1) * u.y + H(1, 2))*norm;
							}
							else
							{
								frame.WarpDx(i, j) = frame.Dx(i, j);
								frame.WarpDy(i, j) = frame.Dy(i, j);
							}
						}
					}
				}

				frame.Interpolate(frameSizeAfterInterpolation, fscale);
			}
			else
			{
				if(frame.NoMotionVectors || (hogInfo.enabled && frame.RawImage.empty()))
				{
					log("#skipping");
					TIMERS.SkippedFrames++;
					continue;
				}
				frame.Interpolate(frameSizeAfterInterpolation, fscale);
			}
			buffer.Update(frame);
			TIMERS.DescriptorComputation.Stop();
		
			if(buffer.AreDescriptorsReady)
			{
				for(int k = 0; k < patchSizes.size(); k++)
				{
					int blockWidth = patchSizes[k].width / cellSize;
					int blockHeight = patchSizes[k].height / cellSize;
					int xStride = opts.Dense ? 1 : blockWidth / 2;
					int yStride = opts.Dense ? 1 : blockHeight / 2;
					log("#cellSize: %d, blockWidth: %d, blockHeight: %d, xStride: %d, yStride: %d", cellSize, blockWidth, blockHeight, xStride, yStride);
					buffer.PrintFullDescriptor(blockWidth, blockHeight, xStride, yStride);
				}
			}
		}
	}
	TIMERS.Everything.Stop();
	TIMERS.Print(rdr.FrameCount);
 }
