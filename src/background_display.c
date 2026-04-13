// background_display.c
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdio.h>
#include <stdint.h>
#include "lcd.h"
#include "background_display.h"

ObstacleRow rows[MAX_ROWS];

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
            rows[i].y += SCROLL_STEP;
            // If row has moved off bottom of screen, mark as inactive
            if (rows[i].y >= LCD_H)
                rows[i].active = false;
        }
    }

    // 3. Redraw all active rows at new position
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

    // Debug:
    // for (int i = 0; i < MAX_ROWS; i++)
    //     printf("row %d: active=%d y=%d\n", i, rows[i].active, rows[i].y);
    // printf("---\n");
}

void show_highscore(int highscore)
{    
    char buffer[20];
    sprintf(buffer, "High Score: %05d", highscore);
    LCD_DrawString(0, 0, BLACK, WHITE, buffer, 12, 0);
}
