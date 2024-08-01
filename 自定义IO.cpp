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
}

using namespace std;

struct BufferData {
	uint8_t* ptr;
	size_t size;
};
int read_pkt(void* opaque,uint8_t* buffer,int bufferSize) {
	BufferData* data = static_cast<BufferData*>(opaque);
	bufferSize = FFMIN(bufferSize, data->size);

	if (!buffer) {
		return AVERROR_EOF;
	}

	cout << "size : " << data->size << "u" << endl;
	memcpy(buffer, data->ptr, bufferSize);
	data->ptr += bufferSize;
	data->size -= bufferSize;

	return bufferSize;
}
int main() {

	AVFormatContext* fmtCtx = nullptr;
	AVIOContext* avioCtx = nullptr;
	uint8_t* buffer = nullptr;
	uint8_t* avioBuffer = nullptr;
	size_t bufferSize = 0;
	size_t avioBufferSize = 4096;

	string inputFile = __FILE__;
	inputFile += "/../Res/audio.mp3";

	int ret = av_file_map(inputFile.c_str(), &buffer, &bufferSize,0,nullptr);
	if (ret < 0) {
		cerr << "文件映射失败" << endl;
		return -1;
	}
	BufferData data;
	data.ptr = buffer;
	data.size = bufferSize;

	fmtCtx = avformat_alloc_context();
	if (!fmtCtx) {
		cerr << "创建失败" << endl;
		return -1;
	}

	avioBuffer = (uint8_t*)av_malloc(avioBufferSize);
	if (!avioBuffer) {
		cerr << "分配失败" << endl;
		return -1;
	}
	avioCtx = avio_alloc_context(avioBuffer, avioBufferSize, 
		0, &data, read_pkt, nullptr, nullptr);

	if (!avioCtx) {
		cerr << "创建失败" << endl;
		return -1;
	}

	fmtCtx->pb = avioCtx;

	ret = avformat_open_input(&fmtCtx, nullptr, nullptr, nullptr);
	if (ret != 0) {
		cerr << "打开失败" << endl;
		return -1;
	}

	avformat_find_stream_info(fmtCtx,nullptr);

	av_dump_format(fmtCtx, 0, inputFile.c_str(), 0);

	avio_context_free(&avioCtx);
	av_file_unmap(buffer, bufferSize);
	avformat_free_context(fmtCtx);

	return 0;
}