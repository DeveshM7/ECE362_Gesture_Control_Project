#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"

// ── Wavetable config ──────────────────────────────────────────
#define N    1000
#define RATE 20000
#define AUDIO_PIN 36

static short int wavetable[N];

// Fixed-point wavetable state (two channels, mixed)
static volatile int step0 = 0, offset0 = 0;
static volatile int step1 = 0, offset1 = 0;

static void init_wavetable(void) {
    for (int i = 0; i < N; i++)
        wavetable[i] = (short)((16383 * sin(2.0 * M_PI * i / N)) + 16384);
}

static void set_freq(int chan, float f) {
    if (chan == 0) {
        if (f == 0.0f) { step0 = 0; offset0 = 0; }
        else            step0 = (int)((f * N / RATE) * (1 << 16));
    } else {
        if (f == 0.0f) { step1 = 0; offset1 = 0; }
        else            step1 = (int)((f * N / RATE) * (1 << 16));
    }
}

// ── PWM ISR — fires at RATE Hz ────────────────────────────────
static void pwm_audio_handler(void) {
    uint slice = pwm_gpio_to_slice_num(AUDIO_PIN);
    uint chan  = pwm_gpio_to_channel(AUDIO_PIN);
    pwm_clear_irq(slice);

    offset0 += step0;
    offset1 += step1;
    if (offset0 >= (N << 16)) offset0 -= (N << 16);
    if (offset1 >= (N << 16)) offset1 -= (N << 16);

    uint32_t samp = ((uint32_t)wavetable[offset0 >> 16] +
                     (uint32_t)wavetable[offset1 >> 16]) / 2;

    uint32_t period = 1000000u / RATE;
    samp = (uint32_t)(((uint64_t)samp * period) >> 16);
    pwm_set_chan_level(slice, chan, samp);
}

static void init_pwm_audio(void) {
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);

    uint slice = pwm_gpio_to_slice_num(AUDIO_PIN);
    uint chan  = pwm_gpio_to_channel(AUDIO_PIN);

    pwm_set_clkdiv(slice, 150);
    pwm_set_wrap(slice, (1000000u / RATE) - 1);
    pwm_set_chan_level(slice, chan, 0);

    init_wavetable();

    pwm_set_irq0_enabled(slice, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP_0, pwm_audio_handler);
    irq_set_enabled(PWM_IRQ_WRAP_0, true);

    pwm_set_enabled(slice, true);
}

// ── Note sequence ─────────────────────────────────────────────
typedef struct { float freq; int duration_ms; } Note;

// Simple melody: C D E F G A B C (major scale)
static const Note melody[] = {
    {261.63f, 400},  // C4
    {293.66f, 400},  // D4
    {329.63f, 400},  // E4
    {349.23f, 400},  // F4
    {392.00f, 400},  // G4
    {440.00f, 400},  // A4
    {493.88f, 400},  // B4
    {523.25f, 600},  // C5
    {0.0f,    300},  // rest
};

/*int main(void) {
    stdio_init_all();
    sleep_ms(1000);
    printf("[SOUND TEST] Starting PWM audio on GP%d\n", AUDIO_PIN);

    init_pwm_audio();

    int n = sizeof(melody) / sizeof(melody[0]);

    while (1) {
        for (int i = 0; i < n; i++) {
            printf("  note %d: %.2f Hz for %d ms\n", i, melody[i].freq, melody[i].duration_ms);
            set_freq(0, melody[i].freq);
            set_freq(1, 0.0f);
            sleep_ms(melody[i].duration_ms);
        }
        sleep_ms(500); // pause before looping
    }
}*/
