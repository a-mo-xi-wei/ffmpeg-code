#include<stdio.h>
#include<libavformat/avformat.h>
#include<libavutil/avutil.h>
#include<libavcodec/avcodec.h>

//----------------------------文件复用-----------------------------------


int main() {
	char errBuf[1024];
	AVFormatContext* inFmtCtx = NULL;
	AVFormatContext* outFmtCtx = NULL;

	AVOutputFormat* outputFmt = NULL;

	AVStream* inStream = NULL;
	AVStream* outStream = NULL;

	char* inputFile = __FILE__; 
	strcat(inputFile, "/../Res/video.mp4");

	//printf("%s\n", inputFile);
	
	//char* outputFile = __FILE__;
	//strcat(outputFile, "/../Res/out.aac");

	char* outputFile = __FILE__;
	strcat(outputFile, "/../Res/out.h264");

	int ret = avformat_open_input(&inFmtCtx,inputFile, NULL, NULL);
	if (ret) {
		av_strerror(ret, errBuf, 1024);
		printf("%s 文件打开失败 ： %s\n", inputFile,errBuf);
		return -1;
	}
	avformat_find_stream_info(inFmtCtx,NULL);

	av_dump_format(inFmtCtx, 0, "video.mp4", 0);

	//int index = av_find_best_stream(inFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	int index = av_find_best_stream(inFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	//if (index == AVERROR_STREAM_NOT_FOUND || index == AVERROR_DECODER_NOT_FOUND) {
	if(index < 0 ){
		av_strerror(index, errBuf, 1024);
		printf("%s\n", errBuf);
		return -1;
	}
	inStream = inFmtCtx->streams[index];
	//找到流之后
	outFmtCtx = avformat_alloc_context();
	if (!outFmtCtx) {
		printf("分配失败\n");
		return -1;
	}
	outputFmt = av_guess_format(NULL, outputFile, NULL);
	if (!outputFmt) {
		printf("获取格式失败\n");
		return -1;
	}
	outFmtCtx->oformat = outputFmt;

	outStream = avformat_new_stream(outFmtCtx, NULL);
	if (!outStream) {
		printf("流分配失败\n");
		return -1;
	}

	//将原视频流的编码器参数拷贝给输出流
	avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
	outStream->codecpar->codec_tag = 0;//附加信息，一般用0

	ret = avio_open2(&outFmtCtx->pb, outputFile, AVIO_FLAG_WRITE, NULL,NULL);
	if (ret < 0) {
		printf("pb字段分配失败\n");
		return -1;
	}
	avformat_write_header(outFmtCtx, NULL);

	AVPacket pkt;
	while (!av_read_frame(inFmtCtx, &pkt)) {
		if (pkt.stream_index != index) {
			continue;
		}
		av_packet_rescale_ts(&pkt, inStream->time_base, outStream->time_base);
		pkt.stream_index = outStream->index;
		//将数据包写入文件
		ret = av_interleaved_write_frame(outFmtCtx, &pkt);
		if (ret<0)break;
		av_packet_unref(&pkt);

	}
	av_write_trailer(outFmtCtx);
	 
	avformat_close_input(&inFmtCtx);
	avformat_free_context(outFmtCtx);


}
