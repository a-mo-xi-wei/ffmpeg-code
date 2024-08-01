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
}

using namespace std;

int main() {

	string inputFile = __FILE__;
	inputFile += "/../Res/out.aac";
	string outputFile = __FILE__;
	outputFile += "/../Res/out1.pcm";

	AVFormatContext* fmtCtx = nullptr;
	int ret = avformat_open_input(&fmtCtx, inputFile.c_str(), nullptr, nullptr);
	if (ret != 0) {
		cerr << "打开失败" << endl;
		return -1;
	}
	
	avformat_find_stream_info(fmtCtx, nullptr);

	int index = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_AUDIO, 
		-1, -1, nullptr,0);
	if (index < 0) {
		cerr << "查找失败" << endl;
		return -1;
	}

	const AVCodec* codec = avcodec_find_decoder(fmtCtx->streams[index]->codecpar->codec_id);
	AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(codecCtx, fmtCtx->streams[index]->codecpar);
	ret = avcodec_open2(codecCtx, codec, nullptr);
	if (ret != 0) {
		cerr << "打开失败" << endl;
		return -1;
	}

	SwrContext* swrCtx = nullptr;
	ret = swr_alloc_set_opts2(&swrCtx,
		&codecCtx->ch_layout,
		AV_SAMPLE_FMT_S16,
		48000,
		&codecCtx->ch_layout,
		codecCtx->sample_fmt,
		codecCtx->sample_rate,
		0,nullptr);
	if (ret != 0) {
		cerr << "创建失败" << endl;
		return -1;
	}

	ret = swr_init(swrCtx);
	if (ret < 0)return -1;

	ofstream outFile(outputFile, ostream::binary);
	if (!outFile.is_open()) {
		return -1;
	}
	AVPacket* pkt = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();

	uint8_t** buffer = nullptr;
	int bufferSize = 0;
	int64_t out_sample = av_rescale_rnd(
		swr_get_delay(swrCtx,codecCtx->sample_rate) + codecCtx->frame_size,
		48000,
		codecCtx->sample_rate,
		AV_ROUND_UP);

	av_samples_alloc_array_and_samples(&buffer, &bufferSize,2,out_sample,
		AV_SAMPLE_FMT_S16,0);
	while (av_read_frame(fmtCtx, pkt) == 0) {
		ret = avcodec_send_packet(codecCtx, pkt);
		if (ret != 0) {
			return -1;
		}
		while (ret == 0) {
			ret = avcodec_receive_frame(codecCtx, frame);
			if (ret < 0)break;
			
			swr_convert(swrCtx, buffer, out_sample, frame->data, 
				frame->nb_samples);

			int useBufferSize = av_samples_get_buffer_size(&bufferSize, 2, 
				out_sample, AV_SAMPLE_FMT_S16, 1);
			if (useBufferSize < 0)return -1;
			outFile.write((char*)buffer[0], useBufferSize);

			av_frame_unref(frame);
		}
		av_packet_unref(pkt);
	}
	ret = avcodec_send_packet(codecCtx, nullptr);
	while (ret == 0) {
		ret = avcodec_receive_frame(codecCtx, frame);
		if (ret < 0)break;

		swr_convert(swrCtx, buffer, out_sample, frame->data,
			frame->nb_samples);

		int useBufferSize = av_samples_get_buffer_size(&bufferSize, 2,
			out_sample, AV_SAMPLE_FMT_S16, 1);
		if (useBufferSize < 0)return -1;
		outFile.write((char*)buffer[0], useBufferSize);

		av_frame_unref(frame);
	}
	av_packet_unref(pkt);

	outFile.close();
	av_packet_free(&pkt);
	av_frame_free(&frame);
	avformat_close_input(&fmtCtx);
	avcodec_free_context(&codecCtx);
	swr_free(&swrCtx);
	return 0;
}