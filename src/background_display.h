// background_display.h
#ifndef BACKGROUND_DISPLAY_H
#define BACKGROUND_DISPLAY_H

#include "lcd.h"
#include <stdbool.h>

// --- defines ---
#define MAX_OBSTACLES_PER_ROW  6
#define MAX_ROWS               8
#define ROW_H                  40
#define OBSTACLE_WIDTH         30
#define N_COLS                 6
#define COL_W                  (LCD_W / N_COLS)
#define PADDING                3
#define SCROLL_STEP            10

// --- pin definitions --
#define PIN_SDI    19
#define PIN_CS     17
#define PIN_SCK    18
#define PIN_DC     20
#define PIN_nRESET 21

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

// --- globals ---
extern ObstacleRow rows[MAX_ROWS];

// --- function declarations ---
void init_spi_lcd();
void generate_row();
void move_rows_down();
void show_highscore(int highscore);

#endif