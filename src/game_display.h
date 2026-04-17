// game_display.h
#ifndef GAME_DISPLAY_H
#define GAME_DISPLAY_H

#include "lcd.h"
#include <stdbool.h>
#include "pico/stdlib.h"

// --- defines ---
#define MAX_OBSTACLES_PER_ROW  6
#define MAX_ROWS               8
#define ROW_H                  40
#define OBSTACLE_WIDTH         30
#define N_COLS                 6
#define COL_W                  (LCD_W / N_COLS)
#define PADDING                3

// --- pin definitions ---
#define PIN_SDI    19
#define PIN_CS     17
#define PIN_SCK    18
#define PIN_DC     20
#define PIN_nRESET 22
#define PIN_PAUSE  21
#define PIN_PLAY   26

// --- structs ---
typedef struct {
    u16 x;
    u16 width;
    bool active;
} Obstacle;

typedef struct {
    int y;
    Obstacle obstacles[MAX_OBSTACLES_PER_ROW];
    bool active;
} ObstacleRow;

typedef enum {
    STATE_MAIN_MENU,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER
} GameState;

// --- globals ---
extern ObstacleRow rows[MAX_ROWS];
extern volatile GameState current_state;
extern int highscore;
extern int curr_score;
extern int scroll_step;
extern int spawn_rate;
extern int scroll_rate;

// --- function declarations ---
void init_spi_lcd();
void init_buttons();
void gpio_callback(uint gpio, uint32_t events);
void generate_row();
void move_rows_down();
void redraw_rows();
void show_score(int score);
void main_menu_display(int highscore);
void play_game_display();
void game_over_display(int final_score);
void pause_game_display();

#endif