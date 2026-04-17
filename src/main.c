#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "gesture.h"
#include "hardware/spi.h"
#include "lcd.h"
#include <stdio.h>
#include <string.h>
#include "game_display.h"
#include "collision.h"

// ── Direction codes (passed through inter-core FIFO) ──────────
#define DIR_UP    1
#define DIR_DOWN  2
#define DIR_LEFT  3
#define DIR_RIGHT 4

// -- globals --
int highscore = 0;
int curr_score = 0;
volatile GameState current_state = STATE_MAIN_MENU;

// Character position
int player_x = 120, player_y = 310; // Start at bottom center of 240 x 320 screen
const int move_step = 20; // Note: Decrement `player_y` to move upward (toward top of screen)

// Character attributes
const int player_size = 10;
const uint16_t player_color = GREEN;
const uint16_t bg_color = WHITE;

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

void start_game_logic() {
    curr_score = 0;
    player_x = 120;
    player_y = 310;
    for (int i = 0; i < MAX_ROWS; i++) {
        rows[i].active = false;
    }

    LCD_Clear(WHITE); 
    generate_row();
    redraw_rows();
}

void draw_player() {
    LCD_DrawFillRectangle(player_x, player_y, player_x + 10, player_y + 10, player_color);
}

void move_character(uint32_t dir) {
    // 1. Erase current position (partial redraw to prevent flicker)
    LCD_DrawFillRectangle(player_x, player_y, player_x + player_size, player_y + player_size, bg_color);

    // 2. Calculate new position
    switch (dir) {
        case DIR_UP:    player_y -= move_step; break;
        case DIR_DOWN:  player_y += move_step; break;
        case DIR_LEFT:  player_x -= move_step; break;
        case DIR_RIGHT: player_x += move_step; break;
    }

    // 3. Prevent movement beyond boundaries of screen
    if (player_x < 0) player_x = 0;
    if (player_x > 240 - player_size) player_x = 240 - player_size;
    if (player_y < 0) player_y = 0;
    if (player_y > 320 - player_size) player_y = 320 - player_size;

    // 4. Draw character at new position
    LCD_DrawFillRectangle(player_x, player_y, player_x + player_size, player_y + player_size, player_color);
}

// ── Core 0 — game loop ────────────────────────────────────────
int main(void) {
    stdio_init_all();
    sleep_ms(2000);  // wait for USB serial to connect

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
        // 1. Input handling (detect gestures to control player movement)
        if (multicore_fifo_rvalid()) {
            uint32_t dir = multicore_fifo_pop_blocking();
            switch (dir) {
                case DIR_UP:    printf(">>> GESTURE: UP\n\n");    break;
                case DIR_DOWN:  printf(">>> GESTURE: DOWN\n\n");  break;
                case DIR_LEFT:  printf(">>> GESTURE: LEFT\n\n");  break;
                case DIR_RIGHT: printf(">>> GESTURE: RIGHT\n\n"); break;
            }

            if (current_state == STATE_PLAYING) {
                move_character(dir);
                draw_player();
                if (check_collision()) current_state = STATE_GAME_OVER;
            }
        }

        // 2. State transitions
        if (current_state != last_state) {
            switch (current_state) {
                case STATE_MAIN_MENU:
                    main_menu_display(highscore);
                    break;

                case STATE_PLAYING:
                    if (last_state == STATE_PAUSED) {
                        LCD_Clear(WHITE); // Redraw rows and score after clearing
                        redraw_rows();
                        show_score(curr_score);
                    } else { // Fresh start
                        start_game_logic();
                    }
                    break;

                case STATE_PAUSED:
                    pause_game_display();
                    break;

                case STATE_GAME_OVER:
                    game_over_display(curr_score);
                    break;
            }
            last_state = current_state;
        }

        // 3. Continuous visual updates
        if (current_state == STATE_PLAYING && last_state == STATE_PLAYING) {
            play_game_display();
            draw_player();
            if (check_collision()) current_state = STATE_GAME_OVER;
        }
    }
}