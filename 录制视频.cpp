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
//ffmpeg -list_devices true -f dshow -i dummy
//ffmpeg -list_options true -f dshow -i video="Integrated Webcam"
int main() {

	string url = __FILE__;
	url += "/../Res/video.yuv";

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

	string device_name("video=");

	for (int i = 0; i < devs->nb_devices; ++i) {
		AVDeviceInfo* dev = devs->devices[i];
		if (dev) {
			if (*dev->media_types == AVMEDIA_TYPE_VIDEO) {
				device_name.append(dev->device_name);
				break;
			}
		}
	}

	avdevice_free_list_devices(&devs);

	AVDictionary* options = nullptr;
	av_dict_set(&options, "pixel_format", "yuyv422", 0);
	av_dict_set(&options, "framerate", "30", 0);
	av_dict_set(&options, "video_size", "640x480", 0);

	AVFormatContext* fmtCtx = nullptr;
	ret = avformat_open_input(&fmtCtx, device_name.c_str(), inputFmt, &options);
	if (ret != 0) {
		cerr << "打开失败" << endl;
		return -1;
	}
	av_dump_format(fmtCtx, 0, device_name.c_str(), 0);

	ofstream outFile(url, ofstream::binary | ostream::out);
	if (!outFile.is_open()) {
		cerr << "文件打开失败" << endl;
		return -1;
	}

	AVPacket pkt;
	auto curTime = chrono::steady_clock::now();

	while (av_read_frame(fmtCtx, &pkt) == 0) {
		cout << pkt.size << endl;
		outFile.write((char*)pkt.data, pkt.size);
		av_packet_unref(&pkt);
		if (chrono::steady_clock::now() - curTime > chrono::seconds(3))break;
	}
	outFile.close();
	avformat_close_input(&fmtCtx);
	return 0;
}