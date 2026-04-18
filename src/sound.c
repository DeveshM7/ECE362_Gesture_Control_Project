#include "sound.h"
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "hardware/adc.h"

// ── Config ────────────────────────────────────────────────────
#define AUDIO_PIN 36
#define N         1000
#define RATE      20000

// ── Wavetable + fixed-point state ────────────────────────────
static short int wavetable[N];
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

// ── PWM ISR ───────────────────────────────────────────────────
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

    uint16_t vol = adc_hw->result & 0xFFF;
    samp = (uint32_t)(((uint64_t)samp * vol) >> 12);

    samp = samp * 5;
    uint32_t wrap = 1000000u / RATE;
    if (samp > wrap) samp = wrap;

    pwm_set_chan_level(slice, chan, samp);
}

// ── Init ──────────────────────────────────────────────────────
void sound_init(void) {
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);

    uint slice = pwm_gpio_to_slice_num(AUDIO_PIN);
    uint chan  = pwm_gpio_to_channel(AUDIO_PIN);

    pwm_set_clkdiv(slice, 150);
    pwm_set_wrap(slice, (1000000u / RATE) - 1);
    pwm_set_chan_level(slice, chan, 0);

    init_wavetable();

    adc_init();
    adc_gpio_init(45);
    adc_select_input(5);
    adc_run(true);

    pwm_set_irq0_enabled(slice, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP_0, pwm_audio_handler);
    irq_set_enabled(PWM_IRQ_WRAP_0, true);

    pwm_set_enabled(slice, true);
}

// ── Sound effects ─────────────────────────────────────────────

// Ascending jingle: C4 E4 G4 C5
void sound_play_start(void) {
    float notes[] = {261.63f, 329.63f, 392.00f, 523.25f};
    int   durations[] = {120, 120, 120, 250};
    int   n = 4;
    for (int i = 0; i < n; i++) {
        set_freq(0, notes[i]);
        set_freq(1, 0.0f);
        sleep_ms(durations[i]);
    }
    set_freq(0, 0.0f);
}

// Short blip: 880 Hz for 40 ms
void sound_play_tick(void) {
    set_freq(0, 880.0f);
    sleep_ms(40);
    set_freq(0, 0.0f);
}

// Descending tone: G4 E4 C4 A3
void sound_play_death(void) {
    float notes[] = {392.00f, 329.63f, 261.63f, 220.00f};
    int   durations[] = {200, 200, 200, 400};
    int   n = 4;
    for (int i = 0; i < n; i++) {
        set_freq(0, notes[i]);
        set_freq(1, 0.0f);
        sleep_ms(durations[i]);
    }
    set_freq(0, 0.0f);
}
