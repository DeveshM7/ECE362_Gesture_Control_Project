#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "gesture.h"
#include "hardware/spi.h"
#include "lcd.h"
#include <stdio.h>
#include <string.h>
#include "background_display.h"

// ── Direction codes (passed through inter-core FIFO) ──────────
#define DIR_UP    1
#define DIR_DOWN  2
#define DIR_LEFT  3
#define DIR_RIGHT 4

// -- Globals for game function --
int highscore = 0;
int curr_score = 0;
volatile GameState current_state = STATE_MAIN_MENU;

static uint32_t gesture_to_int(const char *g) {
    if (g[0] == 'U') return DIR_UP;
    if (g[0] == 'D') return DIR_DOWN;
    if (g[0] == 'L') return DIR_LEFT;
    if (g[0] == 'R') return DIR_RIGHT;
    return 0;
}

// ── Core 1 — gesture detection loop ──────────────────────────
void core1_entry(void) {
    sleep_ms(100);
    printf("[INFO]  Gesture loop running on core %d\n", get_core_num());
    while (1) {
        const char *g = read_gesture();
        if (g) {
            multicore_fifo_push_blocking(gesture_to_int(g));
        }
        sleep_ms(30);
    }
}

// ── Core 0 — game loop ────────────────────────────────────────
int main(void) {
    stdio_init_all();
    sleep_ms(2000);  // wait for USB serial to connect

    printf("\n====================================\n");
    printf("  APDS-9960 Dual-Core Test — RP2350\n");
    printf("====================================\n\n");

    if (!apds_init()) {
        printf("[FAIL]  Sensor init failed. Halting.\n");
        while (1) tight_loop_contents();
    }

    multicore_launch_core1(core1_entry);
    printf("[INFO]  Game loop running on core %d\n", get_core_num());
    printf("[INFO]  Core 1 launched — gesture detection running\n");
    printf("[READY] Swipe your hand over the sensor...\n\n");

    init_spi_lcd();
    init_buttons();
    LCD_Setup();
    LCD_Clear(WHITE);

    srand(time_us_32());

    GameState last_state = -1;

    while (1) {
        // Check if core 1 has detected a gesture
        if (multicore_fifo_rvalid()) {
            uint32_t dir = multicore_fifo_pop_blocking();
            switch (dir) {
                case DIR_UP:    printf(">>> GESTURE: UP\n\n");    break;
                case DIR_DOWN:  printf(">>> GESTURE: DOWN\n\n");  break;
                case DIR_LEFT:  printf(">>> GESTURE: LEFT\n\n");  break;
                case DIR_RIGHT: printf(">>> GESTURE: RIGHT\n\n"); break;
            }
            // TODO: replace printf with move_character(dir)
        }

        // TODO: game loop runs here — rendering, physics, etc.
        // -- Game display --
        if (current_state != last_state)
        {
            GameState state_to_display = current_state; // Store the current state in a local variable

            switch(state_to_display)
            {
                case STATE_MAIN_MENU:
                    main_menu_display(highscore); // Only show main menu once at start
                    break;
                case STATE_PLAYING:
                    if (last_state == STATE_MAIN_MENU || last_state == STATE_GAME_OVER)
                    {
                        curr_score = 0;
                        for (int i = 0; i < MAX_ROWS; i++) // Clear all rows at start of game
                        {
                            rows[i].active = false;
                        }
                        generate_row(); // start with one row already on screen
                        LCD_Clear(WHITE);
                        redraw_rows();
                    }
                    else if (last_state == STATE_PAUSED)
                    {
                        LCD_Clear(WHITE);
                        redraw_rows(); // Redraw rows when resuming from pause
                        show_score(curr_score); // Redraw score when resuming from pause
                    }
                    play_game_display(); // Only show playing screen once when game starts
                    last_state = -1;
                    break;
                case STATE_PAUSED:
                    pause_game_display(); // Only show pause screen once when paused
                    break;
                case STATE_GAME_OVER:
                    game_over_display(curr_score); // Only show game over screen once when game ends
                    break;
            }
            last_state = state_to_display; // Update last_state to the current state
    }
}
