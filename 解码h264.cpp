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
	inFile += "/../Res/out.h264";
	string outFile(__FILE__);
	outFile += "/../Res/out.yuv";
	char err[1024];

	AVFormatContext* fmtCtx = nullptr;
	int ret = avformat_open_input(&fmtCtx, inFile.c_str(), nullptr, nullptr);
	if (ret != 0) {
		av_strerror(ret, err, 1024);
		cerr << err;
		return -1;
	}
	avformat_find_stream_info(fmtCtx, nullptr);

	int index = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	if (index < 0) {
		av_strerror(index, err, 1024);
		cerr << err;
		return -1;
	}
	const AVCodec* codec = avcodec_find_decoder(
		fmtCtx->streams[index]->codecpar->codec_id);

	AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(codecCtx, fmtCtx->streams[index]->codecpar);
	
	ret = avcodec_open2(codecCtx, codec, nullptr);
	if (ret < 0) {
		av_strerror(ret, err, 1024);
		cerr << err;
		return -1;
	}

	ofstream outputFile(outFile, ostream::binary);
	if (!outputFile.is_open()) {
		cerr << "文件打开失败" << endl;
		return -1;
	}

	AVPacket* pkt = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();

	while (av_read_frame(fmtCtx, pkt) >= 0) {
		if (pkt->stream_index != index) {
			continue;
		}

		ret = avcodec_send_packet(codecCtx, pkt);
		if (ret < 0) {
			break;
		}
		while (ret >= 0) {
			ret = avcodec_receive_frame(codecCtx, frame);
			if (ret != 0) {
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;
				}
				av_strerror(ret, err, sizeof(err));
				cerr << err;
				break;
			}

			for (int i = 0; i < frame->height; i++)
			{
				outputFile.write((char*)frame->data[0] +
					i * frame->linesize[0],frame->width );
			}
			for (int i = 0; i < frame->height/2; i++)
			{
				outputFile.write((char*)frame->data[1] +
					i * frame->linesize[1],frame->width / 2);
			}
			for (int i = 0; i < frame->height/2; i++)
			{
				outputFile.write((char*)frame->data[2] +
					i * frame->linesize[2],frame->width / 2);
			}
			av_frame_unref(frame);
		}
		av_packet_unref(pkt);
	}
	//清空编码器
	ret = avcodec_send_packet(codecCtx, pkt);
	while (ret >= 0) {
		ret = avcodec_receive_frame(codecCtx, frame);
		if (ret != 0) {
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				break;
			}
			av_strerror(ret, err, sizeof(err));
			cerr << err;
			break;
		}

		for (int i = 0; i < frame->height; i++)
		{
			outputFile.write((char*)frame->data[0] +
				i * frame->linesize[0], frame->width);
		}
		for (int i = 0; i < frame->height / 2; i++)
		{
			outputFile.write((char*)frame->data[1] +
				i * frame->linesize[1], frame->width / 2);
		}
		for (int i = 0; i < frame->height / 2; i++)
		{
			outputFile.write((char*)frame->data[2] +
				i * frame->linesize[2], frame->width / 2);
		}
		av_frame_unref(frame);
	}
	av_packet_unref(pkt);

	outputFile.close();
	av_packet_free(&pkt);
	av_frame_free(&frame);
	avcodec_free_context(&codecCtx);
	avformat_close_input(&fmtCtx);
	cout << "解码完成" << endl;
	return 0;
}
