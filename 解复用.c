#include<stdio.h>

#include"libavformat/avformat.h"
#include"libavcodec/avcodec.h"
#include<libavutil/avutil.h>

//----------------------------文件解复用-----------------------------------

int main() {

	//创建上下文
	AVFormatContext* fmt_ctx = NULL;
	//打开文件
	int ret = avformat_open_input(&fmt_ctx, "file:E:\\Res\\VideoRes\\video.mp4", NULL, NULL);
	if (ret) {
		printf("打开文件失败 -》 %s\n",av_err2str(ret));
		return -1;
	}

	avformat_find_stream_info(fmt_ctx, NULL);
	//打印信息
	av_dump_format(fmt_ctx, 0, "video.mp4", 0);

	const AVCodec* videoCodec = NULL;
	AVStream* vstream;
	AVStream* astream;
	int videoIndex = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &videoCodec, 0);
	vstream = fmt_ctx->streams[videoIndex];
	
	if (videoIndex == AVERROR_STREAM_NOT_FOUND) {
		puts("没有找到视频流\n");
	}
	else{
		
		printf("FPS : %lf\n", av_q2d(vstream->avg_frame_rate));
		printf("编码器ID : %d\n", vstream->codecpar->codec_id);
		printf("分辨率 : %d x %d\n", vstream->codecpar->width, vstream->codecpar->height);
		printf("视频时长(单位：秒) : %lld\n", fmt_ctx->duration / AV_TIME_BASE);
	}
	const AVCodec* audioCodec = NULL;
	int audioIndex = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &audioCodec, 0);
	astream = fmt_ctx->streams[audioIndex];
	if (audioIndex == AVERROR_STREAM_NOT_FOUND) {
		if (videoIndex == AVERROR_STREAM_NOT_FOUND) {
			puts("没有找到音视频流\n");
			return -1;
		}
		puts("没有找到音频流\n");
	}
	else{
		
		printf("采样率 : %d\n", astream->codecpar->sample_rate);
		printf("采样格式 : %s\n", av_get_sample_fmt_name(astream->codecpar->format));
		printf("声道数 : %d\n", astream->codecpar->ch_layout.nb_channels);
		printf("采样数量 : %d\n", astream->codecpar->frame_size);
		
	}
	
	AVPacket* pkt = av_packet_alloc();
	if (!pkt)return -1;
	int i = 0;
	while(1) {
		if (av_read_frame(fmt_ctx, pkt) < 0) {
			break;
		}
		printf("\n");
		if (pkt->stream_index == audioIndex) {
			printf("音频PTS : %lf\n", pkt->pts * av_q2d(astream->time_base));
			printf("音频DTS : %lf\n", pkt->dts * av_q2d(astream->time_base));
			printf("音频大小 : %d\n", pkt->size);
			printf("音频位置 : %lld\n", pkt->pos);
			printf("音频时长 : %lf\n", pkt->duration * av_q2d(astream->time_base));
		}
		else if (pkt->stream_index == videoIndex) {
			printf("视频PTS : %lf\n", pkt->pts * av_q2d(vstream->time_base));
			printf("视频DTS : %lf\n", pkt->dts * av_q2d(vstream->time_base));
			printf("视频大小 : %d\n", pkt->size);
			printf("视频位置 : %lld\n", pkt->pos);
			printf("视频时长 : %lf\n", pkt->duration * av_q2d(vstream->time_base));
		}
		
		av_packet_unref(pkt);
		if (++i > 10)break;
	}
	avformat_close_input(&fmt_ctx);
	av_packet_free(&pkt);
	return 0;

}