#include<iostream>
#include<fstream>
extern "C" {
#include<libavformat/avformat.h>
#include<libavutil/avutil.h>
#include<libavcodec/avcodec.h>
}

using namespace std;

void writeADTSHeader(ofstream& outFile, const AVCodecContext* codecCtx, const int size) {
	uint8_t adts[7]{ 0 };
	adts[0] = 0xFF;
	adts[1] = 0xF1;
	adts[2] = (codecCtx->profile << 6) + (4 << 2) + (codecCtx->ch_layout.nb_channels >> 2);
	adts[3] = ((codecCtx->ch_layout.nb_channels & 3) << 6) + ((size + 7) >> 11);
	adts[4] = (size + 7) & 0x7FF >> 3;
	adts[5] = (((size + 7) & 7) << 5) + 0x1F;
	adts[6] = 0xFC;
	outFile.write((char*)adts, 7);
}

int main() {
	string inFile(__FILE__);
	inFile += "/../Res/out.pcm";
	string outFile(__FILE__);
	outFile += "/../Res/output.aac";
	char err[1024];

	const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
	if (!codecCtx) { return -1; }
	codecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
	codecCtx->sample_rate = 44100;
	AVChannelLayout ch_lay = AV_CHANNEL_LAYOUT_STEREO;
	av_channel_layout_copy(&codecCtx->ch_layout, &ch_lay);
	codecCtx->bit_rate = 128'000;
	codecCtx->flags = AV_CODEC_FLAG_GLOBAL_HEADER;
	codecCtx->profile = AV_PROFILE_AAC_HE_V2;

	int ret = avcodec_open2(codecCtx, codec, nullptr);
	if (ret < 0) {
		av_strerror(ret, err, sizeof(err));
		cerr << err;
		return -1;
	}

	ifstream inputFile(inFile, ifstream::binary);
	ofstream outputFile(outFile, ofstream::binary);
	if (!inputFile.is_open() || !outputFile.is_open()) {
		cerr << "文件打开失败" << endl;
		return -1;
	}
	AVPacket* pkt = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();
	if (!pkt || !frame) {
		cerr << "内存分配失败";
		return -1;
	}
	frame->format = codecCtx->sample_fmt;
	frame->nb_samples = codecCtx->frame_size;
	frame->sample_rate = codecCtx->sample_rate;
	av_channel_layout_copy(&frame->ch_layout,&codecCtx->ch_layout);

	ret = av_frame_get_buffer(frame, 0);
	if (ret < 0) {
		return -1;
	}
	int fmtByte = av_get_bytes_per_sample(codecCtx->sample_fmt);
	uint8_t* buffer = new uint8_t(fmtByte * codecCtx->ch_layout.nb_channels);
	if (!buffer) { return -1; }
	while (!inputFile.eof()) {
		for (int i = 0; i < (frame->nb_samples * frame->ch_layout.nb_channels) / 2; ++i) {
			inputFile.read((char*)(buffer), fmtByte * 2);
			if (inputFile.eof()) {
				break;
			}
			for (size_t j = 0; j < frame->ch_layout.nb_channels; j++)
			{
				memcpy(frame->data[j] + i * fmtByte, buffer, fmtByte);
			}
		}	
		ret = avcodec_send_frame(codecCtx, frame);
		if (ret < 0)break;
		while (ret >= 0) {
			ret = avcodec_receive_packet(codecCtx, pkt);
			if (ret < 0)break;
			writeADTSHeader(outputFile, codecCtx, pkt->size);
			outputFile.write((char*)pkt->data, pkt->size);
			av_packet_unref(pkt);
		}
		
	}
	//清空解码器
	ret = avcodec_send_frame(codecCtx, nullptr);
	while (ret >= 0) {
		ret = avcodec_receive_packet(codecCtx, pkt);
		if (ret < 0)break;
		writeADTSHeader(outputFile, codecCtx, pkt->size);
		outputFile.write((char*)pkt->data, pkt->size);
		av_packet_unref(pkt);
	}


	inputFile.close();
	outputFile.close();
	av_packet_free(&pkt);  
	av_frame_free(&frame);
	avcodec_free_context(&codecCtx);
	cout << "编码完成" << endl;

	return 0;
}
