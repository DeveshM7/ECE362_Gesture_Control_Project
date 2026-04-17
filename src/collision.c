#include "collision.h"
#include "pico/stdlib.h"
#include "game_display.h"

extern int player_x, player_y;
extern const int player_size;

bool check_collision(void) {
    for (int i = 0; i < MAX_ROWS; i++) {
        if (!rows[i].active) continue;
        for (int col = 0; col < N_COLS; col++) {
            if (!rows[i].obstacles[col].active) continue;
            Obstacle *o = &rows[i].obstacles[col];
            if (player_x < o->x + o->width  &&
                player_x + player_size > o->x &&
                player_y < rows[i].y + ROW_H  &&
                player_y + player_size > rows[i].y)
                return true;
        }
    }
    return false;
}
