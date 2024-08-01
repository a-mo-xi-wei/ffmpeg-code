#include<iostream>
#include<fstream>
extern "C" {
#include<libavformat/avformat.h>
#include<libavutil/avutil.h>
#include<libavcodec/avcodec.h>
}

using namespace std;

int main() {
	string inFile(__FILE__);
	inFile += "/../Res/video.yuv";
	string outFile(__FILE__);
	outFile += "/../Res/out1.h264";
	char err[1024];

	const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
	if (!codecCtx)return -1;

	// 设置视频编码的等级
	codecCtx->profile = AV_PROFILE_H264_HIGH_444;
	// 设置视频级别
	codecCtx->level = 50;
	// 设置视频编码的宽度
	codecCtx->width = 1280;
	// 设置视频编码的高度
	codecCtx->height = 720;
	// 设置视频编码的像素格式
	codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	// 设置视频编码的时间基
	codecCtx->time_base = AVRational{ 1,30 };
	// 设置视频编码的帧率
	codecCtx->framerate = AVRational{ 30,1 };
	// 设置视频编码的 GOP 长度
	codecCtx->gop_size = 30;
	// 设置视频编码的最小帧间隔
	codecCtx->keyint_min = 3;
	// 设置视频编码的最大帧间隔
	codecCtx->max_b_frames = 3;
	// 设置视频编码是否有 B 帧
	codecCtx->has_b_frames = 1;
	// 设置视频编码的参考帧数
	codecCtx->refs = 3;
	// 设置视频编码的码率
	codecCtx->bit_rate = 500000;

	int ret = avcodec_open2(codecCtx, codec, nullptr);
	if (ret < 0) {
		av_strerror(ret, err, sizeof(err));
		cerr << err;
		return -1;
	}

	ifstream inputFile(inFile, ios::binary);
	ofstream outputFile(outFile, ios::binary);
	if (!inputFile || !outputFile) {
		cerr << "文件打开失败" << endl;
		return -1;
	}
	AVPacket* pkt = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();
	if (!pkt || !frame) {
		cerr << "分配失败" << endl;
		return -1;
	}
	frame->height = codecCtx->height;
	frame->width = codecCtx->width;
	frame->format = codecCtx->pix_fmt;

	ret = av_frame_get_buffer(frame, 0);
	if (ret != 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Error allocating frame data\n");
		return -1;
	}

	int yBytes = codecCtx->width * codecCtx->height;
	int pts = 0;

	while (!inputFile.eof()) {
		inputFile.read((char*)frame->data[0], yBytes);
		inputFile.read((char*)frame->data[1], yBytes / 4);
		inputFile.read((char*)frame->data[2], yBytes / 4);

		if (inputFile.eof())
			break;

		frame->pts = pts++;

		ret = avcodec_send_frame(codecCtx, frame);
		if (ret < 0)
		{  
			av_strerror(ret, err, sizeof(err));
			cerr << err << endl;
			break;
		}
		while (ret >= 0) {
			ret = avcodec_receive_packet(codecCtx, pkt);
			if (ret != 0) {
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;
				}
				av_strerror(ret, err, sizeof(err));
				cerr << err;
				break;
			}
			outputFile.write((char*)pkt->data, pkt->size);
			av_packet_unref(pkt);
		}
		//编码不用 av_frame_unref
	}
	//清空编码器
	ret = avcodec_send_frame(codecCtx, nullptr);
	while (ret >= 0) {
		ret = avcodec_receive_packet(codecCtx, pkt);
		if (ret < 0) {
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				break;
			}
			av_strerror(ret, err, sizeof(err));
			cerr << err;
			break;
		}
		outputFile.write((char*)pkt->data, pkt->size);
	}
	inputFile.close();
	outputFile.close();

	av_packet_free(&pkt);
	av_frame_free(&frame);
	avcodec_free_context(&codecCtx);
	cout << "编码完成" << endl;
	return 0;
}
