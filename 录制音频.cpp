#include<iostream>
#include<fstream>
#include<string>
#include<chrono>
extern "C" {
#include"libavformat/avformat.h"
#include"libavcodec/avcodec.h"
#include"libavutil/avutil.h"
#include"libavdevice/avdevice.h"
}

using namespace std;

int main() {

	string url = __FILE__;
	url += "/../Res/out.pcm";

	avdevice_register_all();

	AVDeviceInfoList* devs = nullptr;
	const AVInputFormat* inputFmt = av_find_input_format("dshow");
	if (!inputFmt) {
		//cerr << "未找到输入格式" << endl;
		return -1;
	}

	int ret = avdevice_list_input_sources(inputFmt, nullptr, nullptr, &devs);
	if (ret < 0) {
		//cerr << "设备未找到" << endl;
		return -1;
	}

	string device_name("audio=");

	for (int i = 0; i < devs->nb_devices; ++i) {
		AVDeviceInfo* dev = devs->devices[i];
		if (dev) {
			if (*dev->media_types == AVMEDIA_TYPE_AUDIO) {
				device_name.append(dev->device_name);
				break;
			}
		}
	}

	avdevice_free_list_devices(&devs);

	AVFormatContext* fmtCtx = nullptr;
	ret = avformat_open_input(&fmtCtx, device_name.c_str(), inputFmt, nullptr);
	if (ret != 0) {
		cerr << "打开失败" << endl;
		return -1;
	}
	av_dump_format(fmtCtx, 0, device_name.c_str(), 0);

	ofstream outFile(url, ofstream::binary | ostream::out);
	if(!outFile.is_open()) {
		cerr << "文件打开失败" << endl;
		return -1;
	}

	AVPacket pkt;
	auto curTime = chrono::steady_clock::now();

	while (av_read_frame(fmtCtx, &pkt) == 0) {
		cout << pkt.size << endl;
		outFile.write((char*)pkt.data, pkt.size);
		av_packet_unref(&pkt);
		if (chrono::steady_clock::now() - curTime > chrono::seconds(5))break;

	}
	outFile.close();
	avformat_close_input(&fmtCtx);
	return 0;
}