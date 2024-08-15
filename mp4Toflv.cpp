#include <iostream>
#include <string>
#include <fstream>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
#include <libavutil/error.h>
}

#define GET_DIR(file) (std::string(file)).substr(0, std::string(file).find_last_of("/\\"))

int main(int argc, char *argv[])
{
    AVFormatContext *ifmtCtx = nullptr , *ofmtCtx = nullptr;
    av_log_set_level(AV_LOG_INFO);

    std::string input_file = GET_DIR(__FILE__) + "/../Res/video.mp4";
    std::string output_file = GET_DIR(__FILE__) + "/video.flv";

    std::ofstream outputFile(output_file, std::ios::binary);
    if (!outputFile.is_open()){
        av_log(nullptr,AV_LOG_ERROR,"open output file failed");
        return -1;
    }

    int ret = avformat_open_input(&ifmtCtx, input_file.c_str(), nullptr, nullptr);
    if (ret < 0)
    {
        av_log(nullptr, AV_LOG_ERROR, "Failed open input");
        return -1;
    }
    
    if(avformat_find_stream_info(ifmtCtx, nullptr) < 0){
        av_log(nullptr, AV_LOG_ERROR, "Failed find stream info");
        return -1;
    }
 
    av_dump_format(ifmtCtx, 0, input_file.c_str(), 0);

    ret = avformat_alloc_output_context2(&ofmtCtx,nullptr,nullptr,output_file.c_str());
    if(ret < 0){
        av_log(nullptr, AV_LOG_ERROR, "Failed alloc output context");
        return -1;
    }
    int stream_index = 0;
    int stream_mapping_size = ifmtCtx->nb_streams;
    int* stream_mapping = (int*)av_malloc_array(stream_mapping_size,sizeof(*stream_mapping));
    if(!stream_mapping){
        av_log(nullptr, AV_LOG_ERROR, "Failed alloc stream mapping");
        return -1;
    }

    const AVOutputFormat* ofmt = ofmtCtx->oformat;

    for(int i = 0 ; i < ifmtCtx->nb_streams ; ++i){
        AVStream* in_stream = ifmtCtx->streams[i];
        AVCodecParameters* in_codecpar = in_stream->codecpar;

        if(in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
        in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
        in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE){
            stream_mapping[i] = -1;
            continue;
        }
        
        stream_mapping[i] = stream_index ++;
        
        AVStream* out_stream = avformat_new_stream(ofmtCtx,nullptr);
        if(!out_stream){
            av_log(nullptr, AV_LOG_ERROR, "Failed alloc output stream");
            return -1;
        }
        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if(ret < 0){
            av_log(nullptr, AV_LOG_ERROR, "Failed copy codec parameters");
            return -1;
        }
        
        out_stream->codecpar->codec_tag = 0;
    }

    av_dump_format(ofmtCtx, 0, output_file.c_str(), 1);

    if(!(ofmt->flags & AVFMT_NOFILE)){
        ret = avio_open2(&ofmtCtx->pb, output_file.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
        if(ret < 0){
            av_log(nullptr, AV_LOG_ERROR, "Failed open output file");
            return -1;
        }
    }
    ret = avformat_write_header(ofmtCtx,nullptr);
    if(ret < 0){
        av_log(nullptr, AV_LOG_ERROR, "Failed write header");
        return -1;
    }
    AVPacket* pkt = av_packet_alloc(); // Allocate the AVPacket
    if (!pkt) {
        av_log(nullptr, AV_LOG_ERROR, "Failed to allocate packet");
        return -1;
    }
    while(1){
        AVStream *in_stream , *out_stream;
        ret = av_read_frame(ifmtCtx,pkt);
        if(ret == AVERROR_EOF || ret == AVERROR(EAGAIN)){
            break;
        }
        if(ret < 0){
            av_log(nullptr, AV_LOG_ERROR, "Failed read frame");
            return -1;
        }
        in_stream = ifmtCtx->streams[pkt->stream_index];
        if(pkt->stream_index >= stream_mapping_size ||
        stream_mapping[pkt->stream_index] < 0){
            av_packet_unref(pkt);
            continue;
        }
        pkt->stream_index = stream_mapping[pkt->stream_index];
        out_stream = ofmtCtx->streams[pkt->stream_index];
        //log_packet(ifmtCtx,pkt,"in");
        /* copy packet */
        pkt->pts = av_rescale_q_rnd(pkt->pts,in_stream->time_base,out_stream->time_base,(AVRounding)(AV_ROUND_INF | AV_ROUND_PASS_MINMAX));
        pkt->dts = av_rescale_q_rnd(pkt->dts,in_stream->time_base,out_stream->time_base,(AVRounding)(AV_ROUND_INF | AV_ROUND_PASS_MINMAX));
        pkt->duration = av_rescale_q(pkt->duration,in_stream->time_base,out_stream->time_base);

        pkt->pos = -1;
        //log_packet(ofmtCtx,pkt,"out");
        ret = av_interleaved_write_frame(ofmtCtx, pkt);
        if(ret < 0){
            av_log(nullptr, AV_LOG_ERROR, "Failed write frame");
            return -1;
        }
        av_packet_unref(pkt);
    }
    av_write_trailer(ofmtCtx);
    av_packet_free(&pkt);
    av_free(stream_mapping);
    avformat_free_context(ofmtCtx);
    avformat_close_input(&ifmtCtx);
    outputFile.close();
    return 0;
}
