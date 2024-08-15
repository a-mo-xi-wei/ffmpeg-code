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

// 获取音频采样率索引
int getSampleRateIndex(int sample_rate) {
    switch (sample_rate) {
        case 96000: return 0;
        case 88200: return 1;
        case 64000: return 2;
        case 48000: return 3;
        case 44100: return 4;
        case 32000: return 5;
        case 24000: return 6;
        case 22050: return 7;
        case 16000: return 8;
        case 12000: return 9;
        case 11025: return 10;
        case 8000: return 11;
        case 7350: return 12;
        default: return 15; // reserved
    }
}

// ADTS header generator
void writeADTSHeader(std::ofstream &outFile, const AVCodecParameters *codecPar, const int size)
{
    uint8_t adts[7] = {0};

    int profile = codecPar->profile + 1; // AAC LC (Low Complexity) profile is 1
    int freqIdx = getSampleRateIndex(codecPar->sample_rate);
    int chanCfg = codecPar->ch_layout.nb_channels;

    adts[0] = 0xFF; // Syncword
    adts[1] = 0xF1; // Syncword, MPEG-2 Layer (0 for MPEG-4), protection absent
    adts[2] = (profile << 6) + (freqIdx << 2) + (chanCfg >> 2);
    adts[3] = ((chanCfg & 3) << 6) + ((size + 7) >> 11);
    adts[4] = ((size + 7) & 0x7FF) >> 3;
    adts[5] = (((size + 7) & 7) << 5) + 0x1F;
    adts[6] = 0xFC; // Number of raw data blocks in frame

    outFile.write((char *)adts, 7);
}

int main(int argc, char *argv[])
{
    AVFormatContext *fmt_ctx = nullptr;
    av_log_set_level(AV_LOG_INFO);

    std::string input_file = GET_DIR(__FILE__) + "/../Res/video.mp4";
    std::string output_file = GET_DIR(__FILE__) + "/video.aac";

    av_log(nullptr, AV_LOG_INFO, "%s\n", input_file.c_str());
    char buf[1024];

    int ret = avformat_open_input(&fmt_ctx, input_file.c_str(), nullptr, nullptr);
    if (ret < 0)
    {
        av_strerror(ret, buf, sizeof(buf));
        av_log(nullptr, AV_LOG_ERROR, "can't open input : %s\n", buf);
        return -1;
    }
    av_dump_format(fmt_ctx, 0, input_file.c_str(), 0);

    avformat_find_stream_info(fmt_ctx, nullptr);

    int audio_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_index < 0)
    {
        av_log(nullptr, AV_LOG_ERROR, "can't find best stream");
        avformat_close_input(&fmt_ctx);
        return -1;
    }

    AVPacket *packet = av_packet_alloc();
    if (!packet)
    {
        av_log(nullptr, AV_LOG_ERROR, "Memory allocation failed");
        return -1;
    }

    const AVCodec *codec = avcodec_find_decoder(fmt_ctx->streams[audio_index]->codecpar->codec_id);
    if (!codec)
    {
        av_log(nullptr, AV_LOG_WARNING, "Decoder not found");
        return -1;
    }

    AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, fmt_ctx->streams[audio_index]->codecpar);

    if (avcodec_open2(codecCtx, codec, nullptr) < 0)
    {
        av_log(nullptr, AV_LOG_ERROR, "Failed to open codec");
        return -1;
    }

    std::ofstream outputFile(output_file.c_str(), std::ofstream::binary);
    if (!outputFile.is_open())
    {
        av_log(nullptr, AV_LOG_ERROR, "Failed to open output file");
        return -1;
    }

    while (av_read_frame(fmt_ctx, packet) >= 0)
    {
        if (packet->stream_index == audio_index)
        {
            // Write ADTS header
            writeADTSHeader(outputFile, fmt_ctx->streams[audio_index]->codecpar, packet->size);
            outputFile.write((char *)packet->data, packet->size);
            
        }
        av_packet_unref(packet);
    }
    avformat_close_input(&fmt_ctx);
    av_packet_free(&packet);
    avcodec_free_context(&codecCtx);
    outputFile.close();
    return 0;
}
