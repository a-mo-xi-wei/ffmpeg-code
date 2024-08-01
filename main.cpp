#include<iostream>
#include<fstream>
#include<string>
#include<chrono>
extern "C" {
#include"libavformat/avformat.h"
#include"libavcodec/avcodec.h"
#include"libavutil/avutil.h"
#include"libavdevice/avdevice.h"
#include"libavformat/avio.h"
#include"libavutil/file.h"
#include"libswresample/swresample.h"
#include"libswscale/swscale.h"
#include"libavutil/imgutils.h"
}

using namespace std;

int main() {

	string inputFile = __FILE__;
	inputFile += "/../Res/video.mp4";
	string outputFile = __FILE__;
	outputFile += "/../Res/out.rgb";

	AVFormatContext* fmtCtx = nullptr;
	int ret = avformat_open_input(&fmtCtx, inputFile.c_str(), nullptr, nullptr);
	if (ret != 0) {
		cerr << "打开失败" << endl;
		return -1;
	}

	avformat_find_stream_info(fmtCtx, nullptr);

	int index = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO,
		-1, -1, nullptr, 0);
	if (ret < 0) {
		cerr << "查找失败" << endl;
		return -1;
	}

	const AVCodec* codec = avcodec_find_decoder(
		fmtCtx->streams[index]->codecpar->codec_id);

	AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
	if (!codecCtx) {
		cerr << "分配失败" << endl;
		return -1;
	}
	avcodec_parameters_to_context(codecCtx, fmtCtx->streams[index]->codecpar);
	ret = avcodec_open2(codecCtx, codec, nullptr);
	if (ret < 0) {
		cerr << "打开失败" << endl;
		return -1;
	}
	SwsContext* swsCtx = sws_getContext(codecCtx->width, codecCtx->height,
		codecCtx->pix_fmt, 640, 480, AV_PIX_FMT_RGB24, SWS_BICUBIC, nullptr, nullptr, nullptr);
	if (!swsCtx) {
		return -1;
	}

	uint8_t* buffer[4];
	int lineSize[4];
	int bufferSize;

	bufferSize = av_image_alloc(buffer, lineSize, 640, 480, AV_PIX_FMT_RGB24, 1);
	if (bufferSize < 0)return -1;
	ofstream outFile(outputFile, ofstream::binary);
	if (!outFile.is_open()) {
		cerr << "打开失败" << endl;
		return -1;
	}
	AVPacket* pkt = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();
	if (!pkt || !frame) {
		cerr << "分配失败" << endl;
		return -1;
	}

	while (av_read_frame(fmtCtx, pkt) == 0){
		if (pkt->stream_index != index)continue;

		ret = avcodec_send_packet(codecCtx, pkt);
		if (ret < 0)break;

		while (ret >= 0) {
			ret = avcodec_receive_frame(codecCtx, frame);
			if (ret != 0)break;

			sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height,
				buffer, lineSize);
			outFile.write((char*)buffer[0], bufferSize);
			av_frame_unref(frame);
		}
		av_packet_unref(pkt);
	}
	ret = avcodec_send_packet(codecCtx, nullptr);
	while (ret >= 0) {
		ret = avcodec_receive_frame(codecCtx, frame);
		if (ret != 0)break;

		sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height,
			buffer, lineSize);
		outFile.write((char*)buffer[0], bufferSize);
		av_frame_unref(frame);
	}
	av_packet_unref(pkt);

	av_packet_free(&pkt);
	av_frame_free(&frame);
	avformat_close_input(&fmtCtx);
	avcodec_free_context(&codecCtx);


	return 0;
}