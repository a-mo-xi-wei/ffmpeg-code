#include<iostream>
#include<fstream>
extern "C" {
#include"libavformat/avformat.h"
#include"libavcodec/avcodec.h"
#include"libavutil/avutil.h"
#include"libavdevice/avdevice.h"
}

using namespace std;
 
int main() {
	avdevice_register_all();

	AVDeviceInfoList* devs = nullptr;
	const AVInputFormat* inputFmt = av_find_input_format("dshow");
	if (!inputFmt) {
		cerr << "未找到输入格式" << endl;
		return -1;
	}

	int ret = avdevice_list_input_sources(inputFmt,nullptr,nullptr,&devs);
	if (ret < 0) {
		cerr << "设备未找到" << endl;
		return -1;
	}

	for (int i = 0; i < devs->nb_devices; ++i) {
		AVDeviceInfo* dev = devs->devices[i];
		if (dev) {
			cout << dev->device_name << endl
				<< dev->device_description << endl
				//<< av_get_media_type_string(*dev->media_types) << endl;
				<< dev->media_types << endl;
		}
	}
	avdevice_free_list_devices(&devs);

	return 0;
}