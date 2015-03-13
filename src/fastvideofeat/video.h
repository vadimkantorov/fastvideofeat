#ifdef _MSC_VER
#include "inttypes.h"
#endif

#include <stdint.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/motion_vector.h>
}

#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <string>
#include "common.h"
#include "diag.h"
#include <opencv/cv.h>

using namespace std;
using namespace cv;

#ifndef __FRAME_READER_H__
#define __FRAME_READER_H__

struct MotionVector
{
	int X,Y;
	float Dx,Dy;

	int Mx, My;
	char TypeCode, SegmCode;
	
	static const int NO_MV = -10000;

	bool NoMotionVector()
	{
		return (Dx == NO_MV && Dy == NO_MV) || (Dx == -NO_MV && Dy == -NO_MV);
	}

	bool IsIntra()
	{
		return TypeCode == 'P' || TypeCode == 'A' || TypeCode == 'i' || TypeCode == 'I';
	}
};

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
				AVDictionary *opts = NULL;
				av_dict_set(&opts, "flags2", "+export_mvs", 0);
				if (!pCodec || avcodec_open2(enc, pCodec, &opts) < 0)
					throw std::runtime_error("Codec not found or cannot open codec");

				videoStream = i;
				video_st = pFormatCtx->streams[i];
				//pFrame = avcodec_alloc_frame();
				pFrame = av_frame_alloc();

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
				if(process_frame(&pktCopy))
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
		av_frame_unref(pFrame);

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

	void InitMotionVector(MotionVector& mv, int sx, int sy, int dx, int dy)
	{
		//inverting vectors to match optical flow directions
		dx = -dx;
		dy = -dy;

		mv.X = sx;
		mv.Y = sy;
		mv.Dx = dx;
		mv.Dy = dy;
		mv.Mx = -1;
		mv.My = -1;
		mv.TypeCode = '?';
		mv.SegmCode = '?';
	}

	void ReadMotionVectors(Frame& f)
	{
		// reading motion vectors, see ff_print_debug_info2 in ffmpeg's libavcodec/mpegvideo.c for reference and a fresh doc/examples/extract_mvs.c
		AVFrameSideData* sd = av_frame_get_side_data(pFrame, AV_FRAME_DATA_MOTION_VECTORS);
		
		AVMotionVector* mvs = (AVMotionVector*)sd->data;
		int mbcount = sd->size / sizeof(AVMotionVector);
		MotionVector mv;
		for(int i = 0; i < mbcount; i++)
		{
			AVMotionVector& mb = mvs[i];
			InitMotionVector(mv, mb.src_x, mb.src_y, mb.dst_x - mb.src_x, mb.dst_y - mb.src_y);
			PutMotionVectorInMatrix(mv, f);
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
			res.NoMotionVectors = av_frame_get_side_data(pFrame, AV_FRAME_DATA_MOTION_VECTORS) == NULL;
			res.PictType = av_get_picture_type_char(pFrame->pict_type);
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
