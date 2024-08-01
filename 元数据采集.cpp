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
	inFile += "/../Res/audio.mp3";
	
	AVFormatContext* fmtCtx = nullptr;
	const AVDictionaryEntry* tag = nullptr;
	int ret = avformat_open_input(&fmtCtx,inFile.c_str(),nullptr,nullptr);
	if (ret != 0) {
		cerr << "打开失败" << endl;
		return -1;
	}
	avformat_find_stream_info(fmtCtx, nullptr); 
	
	while ((tag = av_dict_get(fmtCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
		cout << tag->key << "\t" << tag->value << endl;
	}
	avformat_close_input(&fmtCtx);

	return 0;
}
