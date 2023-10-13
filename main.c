#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>

#define PI 3.14159265
#define SAMPLE_RATE 44100
#define CHANNELS 2
#define DURATION 5  // 时间长度（秒）
#define AMPLITUDE 0.7
#define FREQUENCY 440

int main() {
    int err;
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;
    unsigned int sample_rate = SAMPLE_RATE;
    unsigned int channels = CHANNELS;
    snd_pcm_uframes_t buffer_size;
    snd_pcm_uframes_t period_size;
    char *buffer;
    unsigned int samples_played = 0;
    unsigned int total_samples = sample_rate * DURATION;

    // 初始化PCM设备
    err = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        printf("Cannot open PCM device: %s\n", snd_strerror(err));
        return -1;
    }

    // 分配并初始化硬件参数对象
    snd_pcm_hw_params_malloc(&params);
    snd_pcm_hw_params_any(pcm_handle, params);

    // 设置硬件参数
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, channels);
    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &sample_rate, 0);

    // 应用硬件参数
    snd_pcm_hw_params(pcm_handle, params);

    // 获取缓冲区和周期大小
    snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
    snd_pcm_hw_params_get_period_size(params, &period_size, 0);

    // 分配缓冲区内存
    buffer = (char *)malloc(period_size * channels * 2);  // 2字节/样本

    // 生成正弦波音频数据并写入PCM设备
    while (samples_played < total_samples) {
        int i, j;
        short *ptr = (short *)buffer;

        for (i = 0; i < period_size; i++) {
            double value = AMPLITUDE * sin(2 * PI * FREQUENCY * (samples_played + i) / sample_rate);

            for (j = 0; j < channels; j++) {
                *ptr++ = (short)(value * 32767);  // 将样本值转换为16位有符号整数
            }
        }

        // 写入PCM设备
        err = snd_pcm_writei(pcm_handle, buffer, period_size);
        if (err == -EPIPE) {
            printf("Underrun occurred\n");
            snd_pcm_prepare(pcm_handle);  // 处理数据包未完成的情况
        } else if (err < 0) {
            printf("Error writing to PCM device: %s\n", snd_strerror(err));
            break;
        }

        samples_played += period_size;
    }

    // 清理资源并关闭PCM设备
    free(buffer);
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    snd_pcm_hw_params_free(params);

    return 0;
}
