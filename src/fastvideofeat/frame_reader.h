//#define __STDC_CONSTANT_MACROS
#include <stdint.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <string>
#include "common.h"
#include "diag.h"
#include "motion_vector_file_utils.h"
#include <opencv/cv.h>

using namespace std;
using namespace cv;

//following macros come from mpegvideo.h
#define IS_PCM(a)        ((a)&MB_TYPE_INTRA_PCM)
#define IS_INTERLACED(a) ((a)&MB_TYPE_INTERLACED)
#define IS_16X16(a)      ((a)&MB_TYPE_16x16)
#define IS_16X8(a)       ((a)&MB_TYPE_16x8)
#define IS_8X16(a)       ((a)&MB_TYPE_8x16)
#define IS_8X8(a)        ((a)&MB_TYPE_8x8)
#define USES_LIST(a, list) ((a) & ((MB_TYPE_P0L0|MB_TYPE_P1L0)<<(2*(list))))
#define IS_INTRA(a)      ((a)&7)
#define IS_INTRA4x4(a)   ((a)&MB_TYPE_INTRA4x4)
#define IS_DIRECT(a)     ((a)&MB_TYPE_DIRECT2)
#define IS_GMC(a)        ((a)&MB_TYPE_GMC)
#define IS_INTRA16x16(a) ((a)&MB_TYPE_INTRA16x16)
#define IS_ACPRED(a)     ((a)&MB_TYPE_ACPRED)
#define IS_SKIP(a)       ((a)&MB_TYPE_SKIP)

#ifndef __FRAME_READER_H__
#define __FRAME_READER_H__

struct FrameReader
{
	static const int gridStep = 16;
	Size DownsampledFrameSize;
	Size OriginalFrameSize;
	int FrameCount;
	int frameIndex;
	int64_t prev_pts;
	bool ReadRawImages;

	AVFrame         *pFrame;
	AVFormatContext *pFormatCtx;
	SwsContext		*img_convert_ctx;
	AVStream		*video_st;
	AVIOContext		*pAvioContext;
	uint8_t			*pAvio_buffer;
	FILE* in;
	AVFrame rgb_picture;
	int videoStream;

	static int avio_readPacket(void* opaque, uint8_t* buf, int buf_size)
	{
		TIMERS.Reading.Start();
		int res = fread(buf, 1, buf_size, (FILE*)opaque);
		TIMERS.Reading.Stop();
		return res;
	}

	static void av_null_log_callback(void*, int, const char*, va_list)
	{
	}

	void print_ffmpeg_error(int err) // copied from cmdutils.c
	{
		char errbuf[128];
		const char *errbuf_ptr = errbuf;

		if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
			errbuf_ptr = strerror(AVUNERROR(err));
		av_log(NULL, AV_LOG_ERROR, "print_ffmpeg_error: %s\n", errbuf_ptr);
	}
	
	FrameReader(string videoPath, bool readRawImages)
	{
		ReadRawImages = readRawImages;
		pAvioContext = NULL;
		pAvio_buffer = NULL;
		in = NULL;
		frameIndex = 1;
		videoStream = -1;
		pFormatCtx = avformat_alloc_context();

		av_register_all();

		bool readWholeFileInMemory = false; // disabling for now. possibly manual probing is needed when using custom IO: http://cdry.wordpress.com/2009/09/09/using-custom-io-callbacks-with-ffmpeg/
		if(readWholeFileInMemory)
		{
			const int bufSize = 20* (1 << 20);
			pAvio_buffer = (uint8_t*)av_malloc(bufSize);
			FILE* in = fopen(videoPath.c_str(), "rb");
			pAvioContext = avio_alloc_context(
				pAvio_buffer,
				bufSize,
				false,
				in,
				avio_readPacket,
				NULL,
				NULL);

			videoPath = "dummyFileName";
			pFormatCtx->pb = pAvioContext;
		}
		int err = 0;

		if ((err = avformat_open_input(&pFormatCtx, videoPath.c_str(), NULL, NULL)) != 0)
		{
			print_ffmpeg_error(err);
			throw std::runtime_error("Couldn't open file");
		}

		if ((err = avformat_find_stream_info(pFormatCtx, NULL)) < 0)
		{
			print_ffmpeg_error(err);
			throw std::runtime_error("Stream information not found");
		}

		for(int i = 0; i < pFormatCtx->nb_streams; i++)
		{
			AVCodecContext *enc = pFormatCtx->streams[i]->codec;
			if( AVMEDIA_TYPE_VIDEO == enc->codec_type && videoStream < 0)
			{
				// don't care FF_DEBUG_VIS_MV_B_BACK
				//enc->debug_mv = FF_DEBUG_VIS_MV_P_FOR | FF_DEBUG_VIS_MV_B_FOR;
				//enc->debug |= FF_DEBUG_DCT_COEFF;

				AVCodec *pCodec = avcodec_find_decoder(enc->codec_id);

				//if (pCodec->capabilities & CODEC_CAP_TRUNCATED)
				//	pCodecCtx->flags |= CODEC_FLAG_TRUNCATED;

				if (!pCodec || avcodec_open2(enc, pCodec, NULL) < 0)
					throw std::runtime_error("Codec not found or cannot open codec");

				videoStream = i;
				video_st = pFormatCtx->streams[i];
				pFrame = avcodec_alloc_frame();

				int cols = enc->width;
				int rows = enc->height;

				FrameCount = video_st->nb_frames;
				if(FrameCount == 0)
				{
					double frameScale = av_q2d (video_st->time_base) * av_q2d (video_st->r_frame_rate);
					FrameCount = (double)video_st->duration * frameScale;
				}

				DownsampledFrameSize = Size(cols / gridStep, rows / gridStep);
				OriginalFrameSize = Size(cols, rows);

				PixelFormat target = PIX_FMT_BGR24;
				img_convert_ctx = sws_getContext(video_st->codec->width,
					video_st->codec->height,
					video_st->codec->pix_fmt,
					video_st->codec->width,
					video_st->codec->height,
					target,
					SWS_BICUBIC,
					NULL, NULL, NULL);

				avpicture_fill( (AVPicture*)&rgb_picture, NULL,
					target, cols, rows );

				//av_log_set_level(AV_LOG_QUIET);
				//av_log_set_callback(av_null_log_callback);
				break;
			}
		}
		if(videoStream == -1)
			throw std::runtime_error("Video stream not found");
	}

	bool GetNextFrame()
	{
		static bool initialized = false;
		static AVPacket pkt, pktCopy;

		while(true)
		{
			if(initialized)
			{
				if(process_frame(&pktCopy) > 0)
					return true;
				else
				{
					av_free_packet(&pkt);
					initialized = false;
				}
			}

			int ret = av_read_frame(pFormatCtx, &pkt);
			if(ret != 0)
				break;

			initialized = true;
			pktCopy = pkt;
			if(pkt.stream_index != videoStream )
			{
				av_free_packet(&pkt);
				initialized = false;
				continue;
			}
		}

		return process_frame(&pkt);
	}


	bool process_frame(AVPacket *pkt)
	{
		avcodec_get_frame_defaults(pFrame);

		int got_frame = 0;
		int ret = avcodec_decode_video2(video_st->codec, pFrame, &got_frame, pkt);
		if (ret < 0)
			return false;

		ret = FFMIN(ret, pkt->size); /* guard against bogus return values */
		pkt->data += ret;
		pkt->size -= ret;
		return got_frame > 0;
	}

	void PutMotionVectorInMatrix(MotionVector& mv, Frame& f)
	{
		int i_16 = mv.Y / gridStep;
		int j_16 = mv.X / gridStep;

		i_16 = max(0, min(i_16, DownsampledFrameSize.height-1)); 
		j_16 = max(0, min(j_16, DownsampledFrameSize.width-1));

		if(mv.NoMotionVector())
		{
			f.Missing(i_16,j_16) = true;
		}
		else
		{
			f.Dx(i_16, j_16) = mv.Dx;
			f.Dy(i_16, j_16) = mv.Dy;
		}
	}

	void InitMotionVector(MotionVector& mv, int sx, int sy, int mx, int my, int dx, int dy, int mb_type)
	{
		char typeCode = '_';
		if (IS_PCM(mb_type))
			typeCode = 'P';
		else if (IS_INTRA(mb_type) && IS_ACPRED(mb_type))
			typeCode = 'A';
		else if (IS_INTRA4x4(mb_type))
			typeCode = 'i';
		else if (IS_INTRA16x16(mb_type))
			typeCode = 'I';
		else if (IS_DIRECT(mb_type) && IS_SKIP(mb_type))
			typeCode = 'd';
		else if (IS_DIRECT(mb_type))
			typeCode = 'D';
		else if (IS_GMC(mb_type) && IS_SKIP(mb_type))
			typeCode = 'g';
		else if (IS_GMC(mb_type))
			typeCode = 'G';
		else if (IS_SKIP(mb_type))
			typeCode = 'S';
		else if (!USES_LIST(mb_type, 1))
			typeCode = '>';
		else if (!USES_LIST(mb_type, 0))
			typeCode = '<';
		else {
			assert(USES_LIST(mb_type, 0) && USES_LIST(mb_type, 1));
			typeCode = 'X';
		}

		char segmCode = '_';
		// segmentation
		if (IS_8X8(mb_type))
			segmCode = '+';
		else if (IS_16X8(mb_type))
			segmCode = '-';
		else if (IS_8X16(mb_type))
			segmCode = '|';
		else if (IS_INTRA(mb_type) || IS_16X16(mb_type))
			segmCode = ' ';
		else
			segmCode = '?';

		//inverting vectors to match optical flow directions
		dx = -dx;
		dy = -dy;

		mv.X = sx;
		mv.Y = sy;
		mv.Dx = dx;
		mv.Dy = dy;
		mv.Mx = mx*16;
		mv.My = my*16;
		mv.TypeCode = typeCode;
		mv.SegmCode = segmCode;
	}

	void ReadMotionVectors(Frame& f)
	{
		AVCodecContext* pCodecCtx = video_st->codec;

		const int mb_width  = (pCodecCtx->width + 15) / 16;
		const int mb_height = (pCodecCtx->height + 15) / 16;
		const int mb_stride = mb_width + 1;
		const int mv_sample_log2 = 4 - pFrame->motion_subsample_log2;
		const int mv_stride = (mb_width << mv_sample_log2) + (pCodecCtx->codec_id == CODEC_ID_H264 ? 0 : 1);
		const int quarter_sample = (pCodecCtx->flags & CODEC_FLAG_QPEL) != 0;
		const int shift = 1 + quarter_sample;

		typedef short DCTELEM;
		//Mat_<DCTELEM> dct(mb_height*mb_width, 64*6);
		//memcpy(dct.ptr(), pFrame->dct_coeff, sizeof(DCTELEM)*dct.cols);
		MotionVector mv;
		for (int mb_y = 0; mb_y < mb_height; mb_y++)
		{
			for (int mb_x = 0; mb_x < mb_width; mb_x++)
			{
				const int mb_index = mb_x + mb_y * mb_stride;

				if (pFrame->motion_val)
				{
					for (int type = 0; type < 3; type++)
					{
						int direction = 0;
						switch (type) {
						case 0:
							if (//(!(pCodecCtx->debug_mv & FF_DEBUG_VIS_MV_P_FOR)) ||
								(pFrame->pict_type!= AV_PICTURE_TYPE_P))
								continue;
							direction = 0;
							break;
						case 1:
							if (//(!(pCodecCtx->debug_mv & FF_DEBUG_VIS_MV_B_FOR)) ||
								(pFrame->pict_type!= AV_PICTURE_TYPE_B))
								continue;
							direction = 0;
							break;
						case 2:
							if (//(!(pCodecCtx->debug_mv & FF_DEBUG_VIS_MV_B_BACK)) ||
								(pFrame->pict_type!= AV_PICTURE_TYPE_B))
								continue;
							direction = 1;
							break;
						}

						bool good = USES_LIST(pFrame->mb_type[mb_index], direction);
						int dx = NO_MV;
						int dy = NO_MV;

						if (IS_8X8(pFrame->mb_type[mb_index]))
						{
							for (int i = 0; i < 4; i++)
							{
								int sx = mb_x * 16 + 4 + 8 * (i & 1);
								int sy = mb_y * 16 + 4 + 8 * (i >> 1);
								if(good)
								{
									int xy = (mb_x*2 + (i&1) + (mb_y*2 + (i>>1))*mv_stride) << (mv_sample_log2-1);
									dx = (pFrame->motion_val[direction][xy][0]>>shift);
									dy = (pFrame->motion_val[direction][xy][1]>>shift);
								}
								InitMotionVector(mv, sx, sy, mb_x, mb_y, dx, dy, pFrame->mb_type[mb_index]);
								PutMotionVectorInMatrix(mv, f);
							}
						}
						else if (IS_16X8(pFrame->mb_type[mb_index]))
						{
							for (int i = 0; i < 2; i++)
							{
								int sx = mb_x * 16 + 8;
								int sy = mb_y * 16 + 4 + 8 * i;
								if(good)
								{
									int xy = (mb_x*2 + (mb_y*2 + i)*mv_stride) << (mv_sample_log2-1);
									dx = (pFrame->motion_val[direction][xy][0]>>shift);
									dy = (pFrame->motion_val[direction][xy][1]>>shift);

									if (IS_INTERLACED(pFrame->mb_type[mb_index]))
										dy *= 2;
								}

								InitMotionVector(mv, sx, sy, mb_x, mb_y, dx, dy, pFrame->mb_type[mb_index]);
								PutMotionVectorInMatrix(mv, f);
							}
						}
						else if (IS_8X16(pFrame->mb_type[mb_index]))
						{
							for (int i = 0; i < 2; i++) 
							{
								int sx = mb_x * 16 + 4 + 8 * i;
								int sy = mb_y * 16 + 8;
								if(good)
								{
									int xy =  (mb_x*2 + i + mb_y*2*mv_stride) << (mv_sample_log2-1);
									dx = (pFrame->motion_val[direction][xy][0]>>shift);
									dy = (pFrame->motion_val[direction][xy][1]>>shift);

									if (IS_INTERLACED(pFrame->mb_type[mb_index]))
										dy *= 2;
								}
								InitMotionVector(mv, sx, sy, mb_x, mb_y, dx, dy, pFrame->mb_type[mb_index]);
								PutMotionVectorInMatrix(mv, f);
							}
						}
						else
						{
							int sx= mb_x * 16 + 8;
							int sy= mb_y * 16 + 8;
							if(good)
							{
								int xy = (mb_x + mb_y*mv_stride) << mv_sample_log2;
								dx = (pFrame->motion_val[direction][xy][0]>>shift);
								dy = (pFrame->motion_val[direction][xy][1]>>shift);
							}
							InitMotionVector(mv, sx, sy, mb_x, mb_y, dx, dy, pFrame->mb_type[mb_index]);
							PutMotionVectorInMatrix(mv, f);
						}
					}
				}
			}
		}
	}

	void ReadRawImage(Frame& res)
	{
		rgb_picture.data[0] = res.RawImage.ptr();
		sws_scale(img_convert_ctx, pFrame->data,
			pFrame->linesize, 0,
			video_st->codec->height,
			rgb_picture.data, rgb_picture.linesize);
	}

	Frame Read()
	{
		TIMERS.ReadingAndDecoding.Start();
		Frame res(frameIndex, Mat_<float>::zeros(DownsampledFrameSize), Mat_<float>::zeros(DownsampledFrameSize), Mat_<bool>::zeros(DownsampledFrameSize));
		res.RawImage = Mat(OriginalFrameSize, CV_8UC3);

		bool read = GetNextFrame();
		if(read)
		{
			AVPictureType tp = pFrame->pict_type;
			char picType = tp == AV_PICTURE_TYPE_I ? 'I' : tp == AV_PICTURE_TYPE_B ? 'B' : tp == AV_PICTURE_TYPE_P ? 'P' : '?';

			res.NoMotionVectors = picType == 'I';
			//fragile, consult fresh f_select.c and ffprobe.c when updating ffmpeg
			res.PTS = pFrame->pkt_pts != AV_NOPTS_VALUE ? pFrame->pkt_pts : (pFrame->pkt_dts != AV_NOPTS_VALUE ? pFrame->pkt_dts : prev_pts + 1);
			prev_pts = res.PTS;
			if(!res.NoMotionVectors)
				ReadMotionVectors(res);
			if(ReadRawImages)
				ReadRawImage(res);
		}
		else
		{
			res = Frame(res.FrameIndex);
			res.PTS = -1;
		}

		TIMERS.ReadingAndDecoding.Stop();

		frameIndex++;
		return res;
	}

	~FrameReader()
	{
		//causes double free error. av_free(pFrame);
		//sws_freeContext(img_convert_ctx);
		/*avcodec_close(video_st->codec);
		av_close_input_file(pFormatCtx);*/
		/*if(pAvio_buffer)
			av_free(pAvio_buffer);*/
		/*if(pAvioContext)
			av_free(pAvioContext);*/
		if(in)
			fclose(in);
	}
};

#endif
