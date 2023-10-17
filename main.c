#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <sys/ioctl.h>
#include <linux/serial.h>


int err;
snd_pcm_t *pcm_handle;
snd_pcm_hw_params_t *params;
unsigned int frequency = 440;
double amplitude = 0.7;
unsigned int sample_rate = 44100;
unsigned int channels = 1;
snd_pcm_uframes_t buffer_size;
snd_pcm_uframes_t period_size;
char *buffer;
char device[50] = "/dev/ttyUSB0";

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
    snd_pcm_uframes_t actual_period_size;
    unsigned int total_samples = sample_rate * duration;

    // Generate sine wave data & write to PCM device.
    double pi = 3.14159265;
    while (samples_played < total_samples) {
        short *ptr = (short *)buffer;
        int i;
        for (i = 0; i < period_size; i++) {
            double value = amplitude * sin(2 * pi * frequency * (samples_played) / sample_rate);
            for (int j = 0; j < channels; j++) {
                *ptr++ = (short)(value * 32767);  // Convert sample value to 16-bit signed integer.
            }
            samples_played ++;
            if (samples_played == total_samples) {
                break;
            }
        }
        actual_period_size = i;

        // Write to PCM device.
        err = snd_pcm_writei(pcm_handle, buffer, actual_period_size);
        if (err == -EPIPE) {
            printf("Underrun occurred\n");
            snd_pcm_prepare(pcm_handle);  // Handle unfinished data.
        } else if (err < 0) {
            printf("Error writing to PCM device: %s\n", snd_strerror(err));
            break;
        }

    }

    usleep(duration * 1500000);
}

void manual_key()
{
    printf("Manual key.\n");
}

void auto_key()
{
    printf("Auto key.\n");

    int fd = open(device, O_RDWR | O_NOCTTY);  // Open the serial port
    if (fd == -1) {
        perror("Error opening the serial port");
        exit(EXIT_FAILURE);
    }

    int status;
    if (ioctl(fd, TIOCMGET, &status) == -1) {  // Get the modem status
        perror("Error getting modem status");
        close(fd);
        exit(EXIT_FAILURE);
    }

    status |= TIOCM_RTS;  // Set RTS bit to 1 (active/high)

    if (ioctl(fd, TIOCMSET, &status) == -1) {  // Set the modem status
        perror("Error setting modem status");
        close(fd);
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        if (ioctl(fd, TIOCMGET, &status) == -1) {  // Get the modem status
            perror("Error getting modem status");
            close(fd);
            exit(EXIT_FAILURE);
        }

        if (status & TIOCM_CTS) {
            beep(0.21);
        } else if (status & TIOCM_DSR) {
            beep(0.07);
        } else {
            usleep(200000);
        }
    }
    
    close(fd); // Close the serial port
}

void text_to_morse(char *text)
{
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
    printf("Text:\n%s\n\nMorse Code:\n", text);
    for (int i = 0; i < strlen(text); i ++) {
        char c = text[i];
        char *code = " ";
        if (c >= 'A' && c <= 'Z')
            code = morseAZ[c - 'A'];
        if (c >= 'a' && c <= 'z')
            code = morseAZ[c - 'a'];
        if (c >= '0' && c <= '9')
            code = morse09[c - '0'];
        printf("%s ", code);
        fflush(stdout);
        for (int i = 0; i < strlen(code); i ++) {
            if (code[i] == '-')
                beep(0.21);
            else if (code[i] == '.')
                beep(0.07);
            else {
                usleep(100000);
            }
        }
        usleep(200000);
    }
    printf("\n");
}

void print_help_and_exit(char *argv0)
{
    printf("This program works in one of the three mode: manual key, double paddle key, or send text directly.\n\n");
    printf("Program options:\n\n");
    printf("At lease, one of -t or -k is required.\n\n");
    printf("    -t \"CQ CQ\"     :  text to send\n");
    printf("    -k 2           :  Morse key, 1 for manual key, or 2 for double paddle key\n");
    printf("    -d /dev/ttyS0  :  serial device connecting the Morse key, default to /dev/ttyUSB0\n");
    printf("    -f 600         :  audio frequency, default to 440 Hz\n");
    printf("    -h             :  print this help\n\n");

    printf("Examples:\n\n");
    printf("Send text          :  %s -t \"CQ CQ DE ...\"\n", argv0);
    printf("Manual key         :  %s -k 1\n", argv0);
    printf("Double paddle key  :  %s -k 2\n\n", argv0);


    exit(EXIT_FAILURE);
}

void term(int signum)
{
   exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    printf("*** Morse Code Tool ***\n\n");

    // Ctrl-C & kill signal handlers.
    struct sigaction action = {0};
    action.sa_handler = term;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    // Cleanup at exit.
    atexit(cleanup);

    // Init.
    if ((err = init()))
        return err;

    // Text to convert to Morse code.
    char text[256] = "CQ CQ CQ";

    int c = 0, keys = 0;
    bool send_text = false;
    while((c = getopt(argc, argv, "f:d:k:t:h")) != -1) {
        switch(c) {
        case 'f':
            frequency = atoi(optarg);
            break;
        case 'd':
            strncpy(device, optarg, 50);
            break;
        case 'k':
            keys = atoi(optarg);
            break;
        case 't':
            send_text = true;
            strncpy(text, optarg, 256);
            break;
        case 'h':
        default:
            print_help_and_exit(argv[0]);
        }
    }

    if (send_text) {
        text_to_morse(text);
    } else {
        switch (keys) {
        case 1:
            manual_key();
            break;
        case 2:
            auto_key();
            break;
        default:
            print_help_and_exit(argv[0]);
        }
    }

    return 0;
}
