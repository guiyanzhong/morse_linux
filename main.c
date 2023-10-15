#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>

#define AMPLITUDE 0.7
#define FREQUENCY 440

int err;
snd_pcm_t *pcm_handle;
snd_pcm_hw_params_t *params;
unsigned int frequency = 440;
unsigned int sample_rate = 44100;
unsigned int channels = 1;
snd_pcm_uframes_t buffer_size;
snd_pcm_uframes_t period_size;
char *buffer;

int init()
{
    // Initialize PCM device.
    err = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        printf("Cannot open PCM device: %s\n", snd_strerror(err));
        return -1;
    }

    // Allocate & initialize hardware parameter object.
    snd_pcm_hw_params_malloc(&params);
    snd_pcm_hw_params_any(pcm_handle, params);

    // Set hardware parameters.
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, channels);
    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &sample_rate, 0);

    // Apply hardware parameters.
    snd_pcm_hw_params(pcm_handle, params);

    // Get buffer & period sizes.
    snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
    snd_pcm_hw_params_get_period_size(params, &period_size, 0);

    // Allocate buffer memory.
    buffer = (char *)malloc(period_size * channels * 2);  // 2 bytes per sample.

    return 0;
}

void cleanup()
{
    free(buffer);
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    snd_pcm_hw_params_free(params);
}

void beep(double duration)
{
    snd_pcm_prepare(pcm_handle);

    unsigned int samples_played = 0;
    unsigned int total_samples = sample_rate * duration;

    // Generate sine wave data & write to PCM device.
    double pi = 3.14159265;
    while (samples_played < total_samples) {
        short *ptr = (short *)buffer;
        for (int i = 0; i < period_size; i++) {
            double value = AMPLITUDE * sin(2 * pi * FREQUENCY * (samples_played + i) / sample_rate);
            for (int j = 0; j < channels; j++) {
                *ptr++ = (short)(value * 32767);  // Convert sample value to 16-bit signed integer.
            }
        }

        // Write to PCM device.
        err = snd_pcm_writei(pcm_handle, buffer, period_size);
        if (err == -EPIPE) {
            printf("Underrun occurred\n");
            snd_pcm_prepare(pcm_handle);  // Handle unfinished data.
        } else if (err < 0) {
            printf("Error writing to PCM device: %s\n", snd_strerror(err));
            break;
        }

        samples_played += period_size;
    }

    usleep(duration * 1500000);
}

int main(int argc, char* argv[]) {
    // Text to convert to Morse code.
    char text[256] = "CQ CQ DE BH4FYQ K";

    int c = 0;
    while((c = getopt(argc, argv, "hf:t:")) != -1) {
        switch(c) {
        case 'h':
            printf("Usage: %s -f 600 -t \"CQ CQ DE ...\"\n", argv[0]);
            return 0;
        case 'f':
            strncpy(text, optarg, 256);
            break;
        case 't':
            strncpy(text, optarg, 256);
            break;
        default:
            return 0;
        }
    }

    printf("%s\n", text);

    // Init.
    if ((err = init()))
        return err;

    // Morse codes for alphabets
    char *morseAZ[] = {
        ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..",   // A-I
        ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", // J-R
        "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.."         // S-Z
    };

    // Morse codes for digits
    char *morse09[] = {
        "-----", ".----", "..---", "...--", "....-",                     // 0-4
        ".....", "-....", "--...", "---..", "----."                      // 5-9
    };

    // Beep.
    for (int i = 0; i < strlen(text); i ++) {
        char c = text[i];
        char *code = " ";
        if (c >= 'A' && c <= 'Z')
            code = morseAZ[c - 'A'];
        if (c >= 'a' && c <= 'z')
            code = morseAZ[c - 'a'];
        if (c >= '0' && c <= '9')
            code = morse09[c - '0'];
        printf("%c %s\n", c, code);
        fflush(stdout);
        for (int i = 0; i < strlen(code); i ++) {
            if (code[i] == '-')
                beep(0.2);
            else if (code[i] == '.')
                beep(0.07);
            else {
                usleep(100000);
            }
        }
        usleep(200000);
    }
    printf("\n");

    // Clean up resources & close PCM device.
    cleanup();

    return 0;
}
