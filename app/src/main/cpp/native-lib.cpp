#include <jni.h>
#include <string>
#include <unistd.h>
#include <android/native_window_jni.h>
#include <zconf.h>

#include <android/log.h>

#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"diaochan",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"diaochan",FORMAT,##__VA_ARGS__);

#define MAX_AUDIO_FRAME_SIZE 48000 * 4

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
// 重采样 （编码采样率同播放的采样率不同）    
#include <libswresample/swresample.h>
}


extern "C"
JNIEXPORT void JNICALL
Java_com_diaochan_audiodemo_WangyiPlayer_sound(JNIEnv *env, jobject thiz, jstring input_,
                                               jstring output_) {

    const char *input = env->GetStringUTFChars(input_, 0);
    const char *output = env->GetStringUTFChars(output_, 0);

    int ret;
    /**
     * FFmpeg 视频绘制开始
     */
    avformat_network_init();//初始化网络模块

    /**
     * 第一步：获取AVFormatContext (针对当前视频的 总上下文)
     */
    AVFormatContext *formatContext = avformat_alloc_context();

    //打开音频文件
    if (avformat_open_input(&formatContext, input, NULL, NULL) != 0) {
        LOGI("%s", "无法打开音频文件");
        return;
    }

    // 获取输入的文件信息
    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        LOGI("%s", "无法获取输入的文件信息");
        return;
    }


    // 遍历上下文中的所有流，找到音频流
    int audio_stream_index = -1;
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            break;
        }
    }

    // 获取音频流解码参数 AVCodecParameters
    AVCodecParameters *codecpar = formatContext->streams[audio_stream_index]->codecpar;

    /**
     * 第二步：根据AVCodecParameters中的id,获取解码器AVCodec  
     * 并创建解码器上下文 AVCodecContext
     * 
     * 【带数字的函数，越大数字表示越新】
     */
    AVCodec *avcodec = avcodec_find_decoder(codecpar->codec_id);
    AVCodecContext *codecContext = avcodec_alloc_context3(avcodec);

    //将解码器参数传入到上下文
    // 这样解码器上下文AVCodecContext 和 解码器AVCodec 都有参数的信息
    avcodec_parameters_to_context(codecContext, codecpar);

    /**
     * 第三步：打开解码器
     */
    avcodec_open2(codecContext, avcodec, NULL);

    /**
     * 第四步：解码 YUV数据（存在于AVPacket）
     */
    AVPacket *packet = av_packet_alloc();



    /**
     *  初始化 音频转换上下文SwrContext
     *  
     * （通过转换上下文SwrContext 将frame转换为PCM音频数据）
     * 
     *  
     *  1.设置输入/输出 采样频率、采样位、通道数
     *  2.初始化转换器（其他默认项配置）
     */
    SwrContext *swrContext = swr_alloc();

    /**输入信息*/
    //采样格式
    AVSampleFormat in_sample = codecContext->sample_fmt;
    //采样率
    int in_sample_rate = codecContext->sample_rate;
    //声道布局
    uint64_t in_ch_layout = codecContext->channel_layout;

    /**输出信息*/
    AVSampleFormat out_sample = AV_SAMPLE_FMT_S16;//16位
    int out_sample_rate = 44100;//采样频率44100
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;//双通道

    //设置输入/输出 采样频率、采样位、通道数
    swr_alloc_set_opts(swrContext,
                       out_ch_layout, out_sample, out_sample_rate,
                       in_ch_layout, in_sample, in_sample_rate,
                       0, NULL);
    swr_init(swrContext);


    /**
     * 创建一个缓冲区接收 
     */
    //设置音频缓冲区16bit 44100 PCM数据 (通道数 * 采样频率)
    uint8_t *out_buff = static_cast<uint8_t *>(av_malloc(2 * 44100));
    //创建一个输出文件
    FILE *fp_pcm = fopen(output, "wb");

    //从音频流中读取一个数据包 返回值小于0表示读取完成或错误
    // AVPacket: 压缩数据
    // AVFrame : 未压缩数据
    int count = 0;
    while (av_read_frame(formatContext, packet) >= 0) {
        avcodec_send_packet(codecContext, packet);
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            //重试
            continue;
        } else if (ret < 0) {
            //结束位
            LOGE("解码完成");
            break;
        }


        if (packet->stream_index == audio_stream_index && ret == 0) {
            LOGE("正在解码%d", count++);
        }


        /**
         * 第六步：将frame转换为PCM音频数据
         * 
         * AVFrame --> 统一格式
         */
        // 参数1：转换上下文
        // 参数2：缓冲区
        // 参数3：缓冲区大小
        /**输入源信息*/
        // 参数4：当前帧数据
        // 参数4：当前帧数据采样率
        swr_convert(swrContext, &out_buff, 2 * 44100,
                    (const uint8_t **) frame->data, frame->nb_samples);

        // 从通道布局中获取通道数 （这里是2）
        int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);

        /**
         * 转换完成后，将缓冲区out_buff内容对齐，得到实际缓冲大小
         * 对齐需要 目标采样率和源采样率 以及通道数
         */
        int out_buff_size = av_samples_get_buffer_size(NULL, out_channel_nb,
                                                       frame->nb_samples, out_sample, 1);

        LOGE("该帧转换后输出字节数：%d", out_buff_size);
        //写文件 
        //参数2：音频单位1个字节 像素是4个字节
        //参数3：每次写入的大小
        fwrite(out_buff, 1, out_buff_size, fp_pcm);

    }

    /**
     * 第七步：资源释放
     */
    fclose(fp_pcm);
    av_free(out_buff);
    swr_free(&swrContext);
    avcodec_close(codecContext);
    avformat_close_input(&formatContext);

    env->ReleaseStringUTFChars(input_, input);
    env->ReleaseStringUTFChars(output_, output);
}