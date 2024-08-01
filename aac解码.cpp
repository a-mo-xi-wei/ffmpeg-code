#include<iostream>
#include<fstream>
extern"C" {
#include<libavformat/avformat.h>
#include<libavutil/avutil.h>
#include<libavcodec/avcodec.h>
}

using namespace std;

int main() {
	string inFile(__FILE__);
	inFile += "/../Res/out.aac";
	string outFile(__FILE__);
	outFile += "/../Res/out.pcm";
	char err[1024];

	AVFormatContext* fmtCtx = nullptr;
	int ret = avformat_open_input(&fmtCtx, inFile.c_str(), nullptr, nullptr);
	if (ret != 0) {
		av_strerror(ret, err, sizeof(err));
		cerr << err << "******";
		return -1;
	}

	avformat_find_stream_info(fmtCtx, nullptr);

	int index = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
	if (index < 0) {
		av_strerror(index, err, sizeof(err));
		cerr << err << "******";
		return -1;
	}

	const AVCodec* codec = avcodec_find_decoder(fmtCtx->streams[index]->codecpar->codec_id);
	if (!codec) { cerr << "无解码器"; return -1; }
	AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
	if (!codecCtx) { cerr << "解码器上下文分配错误"; return -1; }
	avcodec_parameters_to_context(codecCtx, fmtCtx->streams[index]->codecpar);
	ret = avcodec_open2(codecCtx, codec, nullptr);
	if (ret < 0) {
		av_strerror(ret, err, sizeof(err));
		cerr << err << "******";
		return -1;
	}

	std::ofstream outputFile(outFile.c_str(), std::ofstream::binary);
	if (!outputFile.is_open()) {
		cerr << "文件打开失败" << endl;
		return -1;
	}

	AVPacket* pkt = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();
	if (!pkt || !frame){ cerr << "pkt 或 frame 分配错误"; return -1;}
	
	while (av_read_frame(fmtCtx, pkt) >= 0) {
		ret = avcodec_send_packet(codecCtx, pkt);
		if (ret < 0) { break; }
		while (ret >= 0) {
			ret = avcodec_receive_frame(codecCtx, frame);
			if (ret < 0) {
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;
				}
				av_strerror(ret, err, sizeof(err));
				cerr << err;
				break;
			}
			int fmtByte = av_get_bytes_per_sample((AVSampleFormat)(frame->format));
			for (int i = 0; i < frame->nb_samples; i++) {
				for (int j = 0; j < frame->ch_layout.nb_channels; j++) {
					outputFile.write((char*)frame->data[j] + fmtByte * i, fmtByte);
					//outputFile.write(static_cast<char*>(frame->data[j]) + fmtByte * i, fmtByte);

				}
			}
			av_frame_unref(frame);
		}
		av_packet_unref(pkt);
	}
	//编码器刷新
	ret = avcodec_send_packet(codecCtx, nullptr);//发送一个空包
	if (ret < 0) { return -1; }
	while (ret >= 0) {
		ret = avcodec_receive_frame(codecCtx, frame);
		if (ret < 0) {
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				break;
			}
			av_strerror(ret, err, sizeof(err));
			cerr << err;
			break;
		}
		int fmtByte = av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format));
		for (int i = 0; i < frame->nb_samples; i++) {
			for (int j = 0; j < frame->ch_layout.nb_channels; j++) {
				outputFile.write((char*)frame->data[j] + fmtByte * i, fmtByte);
				//outputFile.write(static_cast<char*>(frame->data[j]) + fmtByte * i, fmtByte);
			}
		}
		av_frame_unref(frame);
	}
	 

	cout << "解码完成" << endl;

	outputFile.close();
	av_packet_free(&pkt);
	av_frame_free(&frame);
	avformat_close_input(&fmtCtx);
	avcodec_free_context(&codecCtx);

	return 0;
}
