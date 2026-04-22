// game_display.c
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdio.h>
#include <stdint.h>
#include "lcd.h"
#include "game_display.h"
#include "sound.h"
#include "main_menu.h"
#include "game_paused.h"
#include "game_over.h"
#include "file.h"

ObstacleRow rows[MAX_ROWS];


void gpio_callback(uint gpio, uint32_t events);

// ------------------------------------------------------------
//  SPI LCD initialization and button setup
// ------------------------------------------------------------

void init_spi_lcd() {
    gpio_set_function(PIN_CS, GPIO_FUNC_SIO);
    gpio_set_function(PIN_DC, GPIO_FUNC_SIO);
    gpio_set_function(PIN_nRESET, GPIO_FUNC_SIO);

    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_nRESET, GPIO_OUT);

    gpio_put(PIN_CS, 1); // CS high
    gpio_put(PIN_DC, 0); // DC low
    gpio_put(PIN_nRESET, 1); // nRESET high

    // initialize SPI0 with 48 MHz clock
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SDI, GPIO_FUNC_SPI);
    spi_init(spi0, 100 * 1000 * 1000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

void init_buttons()
{
    gpio_set_function(PIN_PLAY, GPIO_FUNC_SIO);
    gpio_set_function(PIN_PAUSE, GPIO_FUNC_SIO);

    gpio_set_dir(PIN_PLAY, GPIO_IN);
    gpio_set_dir(PIN_PAUSE, GPIO_IN);

    gpio_pull_down(PIN_PLAY);
    gpio_pull_down(PIN_PAUSE);

    gpio_set_irq_enabled_with_callback(PIN_PLAY, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    gpio_set_irq_enabled(PIN_PAUSE, GPIO_IRQ_EDGE_RISE, true);
}

// ------------------------------------------------------------
//  Interrupt setup
// ------------------------------------------------------------

void gpio_callback(uint gpio, uint32_t event_mask)
{
    gpio_acknowledge_irq(gpio, event_mask); // Acknowledge interrupt

    static uint32_t last_interrupt_time = 0; // Only update time on successful state change
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (now - last_interrupt_time < 250) { // Slightly longer debounce
        return;
    }
    
    if (gpio == PIN_PLAY) { // Handle play button press
        if (current_state != STATE_PLAYING) { // Start game
            current_state = STATE_PLAYING;
            last_interrupt_time = now;
        }
    } else if (gpio == PIN_PAUSE) { // Handle pause button press
        if (current_state == STATE_PLAYING) { // Pause game
            current_state = STATE_PAUSED;
            last_interrupt_time = now;
        } else { // Return to main menu
            current_state = STATE_MAIN_MENU;
            last_interrupt_time = now;
        }
    }
}

// ------------------------------------------------------------
//  Game display functions
// ------------------------------------------------------------

// --- obstacle generation and movement ---

int scroll_step = 5; // Global variable for scroll step, can be adjusted for difficulty
int spawn_rate = 14500000; // Microseconds between spawns, can be adjusted for difficulty
int scroll_rate = 750000; // Microseconds between scrolls, can be adjusted for difficulty

void generate_row()
{
    for (int i = 0; i < MAX_ROWS; i++) 
    {
        if (!rows[i].active) // find the first inactive row
        {
            // Initialize new row
            ObstacleRow *new_row = &rows[i];
            new_row->y = 0; // Start at top of screen (try to figure out how to gradually bring onto screen?)
            new_row->active = true;

            // Pick gap position
            int gap = rand() % N_COLS;
            
            // Pick confirmed obstacle position
            int confirmed_obstacle;
            do {
                confirmed_obstacle = rand() % N_COLS;
            } while (confirmed_obstacle == gap);
            
            // Generate obstacles for each column
            for (int col = 0; col < N_COLS; col++) 
            {
                if (col == gap) // Leave gap empty
                {
                    new_row->obstacles[col].active = false;
                }
                else if (col == confirmed_obstacle) // Ensure at least one obstacle
                {
                    new_row->obstacles[col].x = col * COL_W + PADDING;
                    new_row->obstacles[col].width = COL_W - 2 * PADDING;
                    new_row->obstacles[col].active = true;
                }
                // 50% chance to create an obstacle for all other positions, otherwise leave empty
                else if (rand() % 2) 
                {
                    new_row->obstacles[col].x = col * COL_W + PADDING;
                    new_row->obstacles[col].width = COL_W - 2 * PADDING;
                    new_row->obstacles[col].active = true;
                }
                else
                {
                    new_row->obstacles[col].active = false;
                }
            }
            return;
        }
    }
}

void move_rows_down()
{
    // 1. Erase all active rows at current position
    for (int i = 0; i < MAX_ROWS; i++)
    {
        if (rows[i].active && rows[i].y >= 0)
        {
            LCD_DrawFillRectangle(0, (u16)rows[i].y, LCD_W, (u16)(rows[i].y + ROW_H), WHITE); // Erase with background color
        }
    }

    // 2. Move all rows down by scroll step
    for (int i = 0; i < MAX_ROWS; i++)
    {
        if (rows[i].active)
        {
            rows[i].y += scroll_step;
            // If row has moved off bottom of screen, mark as inactive
            if (rows[i].y >= LCD_H)
                rows[i].active = false;
        }
    }

    // 3. Redraw all active rows at new position
    redraw_rows();

    // Debug:
    // for (int i = 0; i < MAX_ROWS; i++)
    //     printf("row %d: active=%d y=%d\n", i, rows[i].active, rows[i].y);
    // printf("---\n");
}

void redraw_rows()
{
    for (int i = 0; i < MAX_ROWS; i++)
    {
        if (rows[i].active && rows[i].y >= 0)
        {
            for (int col = 0; col < N_COLS; col++)
            {
                if (rows[i].obstacles[col].active)
                {
                    LCD_DrawFillRectangle(
                        rows[i].obstacles[col].x,
                        (u16)rows[i].y,
                        rows[i].obstacles[col].x + rows[i].obstacles[col].width,
                        (u16)(rows[i].y + ROW_H),
                        YELLOW
                    );
                }
            }
        }
    }
}

// --- score and menu displays ---

void show_score(int score)
{    
    char buffer[20];
    sprintf(buffer, "Score: %05d", score);
    LCD_DrawString(1, 0, BLACK, WHITE, buffer, 12, 0);
}

void main_menu_display(int highscore)
{
    LCD_Clear(WHITE);
    Picture pic;
    pic.width = 240;
    pic.height = 320;
    pic.pixel_data = (unsigned char*)main_menu_map; // This is the array from main_menu.c
    LCD_DrawPicture(0, 0, &pic);

    char buffer1[60];
    char buffer2[60];

    if (leaderboard_count > 0) { 
        for (int i = 0; i < leaderboard_count; i++) {
            sprintf(buffer1, "%d", leaderboard[i].username);
            sprintf(buffer2, "-- %5d", i + 1, leaderboard[i].score);
            LCD_DrawString(60, 128 + i * 30, BLACK, WHITE, buffer1, 16, 0);
            LCD_DrawString(145, 128 + i * 30, BLACK, WHITE, buffer2, 16, 0);
        }
    }
}

// Static variables preserve values between function calls
static uint32_t last_spawn = 0;
static uint32_t last_scroll = 0;
static bool was_paused = false;

void play_game_display() {   
    uint32_t now = time_us_32();
    show_score(curr_score);

    if (current_state != STATE_PLAYING) {
        last_spawn = now;
        last_scroll = now;
        was_paused = true; // Set flag
        return;
    }

    if (was_paused) {
        last_spawn = now;
        last_scroll = now;
        was_paused = false; // Reset flag
    }

    if (last_spawn == 0) { // Initialize timers on first run
        last_spawn = now;
        last_scroll = now;
    }

    if (now - last_spawn >= spawn_rate) {
        generate_row();
        last_spawn = now;
    }

    if (now - last_scroll >= scroll_rate) {
        move_rows_down();
        sound_play_tick();
        last_scroll = now;
        curr_score++;
        // Difficulty scaling
        if (curr_score % 100 == 0)
        {
            scroll_step = scroll_step + (scroll_step / 10);
            spawn_rate = spawn_rate - (spawn_rate / 10);
            scroll_rate = scroll_rate - (scroll_rate / 10);
        }
        show_score(curr_score);
    }
}

void game_over_display(int final_score)
{
    LCD_DrawFillRectangle(50, 100, 190, 230, WHITE);
    
    char buffer[64];
    sprintf(buffer, "%05d", final_score);
    Picture pic;
    pic.width = 140;
    pic.height = 130;
    pic.pixel_data = (unsigned char*)game_over_map;
    LCD_DrawPicture(50, 100, &pic);
    LCD_DrawString(119, 163, BLACK, WHITE, buffer, 16, 0);
    
    if (final_score > highscore)
    {
        highscore = final_score;
    }
}

void pause_game_display()
{
    LCD_DrawFillRectangle(60, 110, 180, 215, WHITE);

    Picture pic;
    pic.width = 120;
    pic.height = 105;
    pic.pixel_data = (unsigned char*)game_paused_map;
    LCD_DrawPicture(60, 110, &pic);
}