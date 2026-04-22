#include "ff.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define MAX_SCORES 10
#define FILE_NAME "scores.txt"

// Helper to sort scores descending
static void sort_scores(int *scores, int count) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (scores[j] > scores[i]) {
                int temp = scores[i];
                scores[i] = scores[j];
                scores[j] = temp;
            }
        }
    }
}

// MAIN FUNCTION YOU NEED
FRESULT leaderboard_submit_score(int score, bool *made_top10) {
    FIL file;
    FRESULT fr;
    char line[32];

    int scores[MAX_SCORES + 1];
    int count = 0;

    *made_top10 = false;

    // 1. Open file (create if it doesn't exist)
    fr = f_open(&file, FILE_NAME, FA_READ | FA_OPEN_ALWAYS);
    if (fr != FR_OK) return fr;

    // 2. Read existing scores
    while (f_gets(line, sizeof(line), &file)) {
        if (count < MAX_SCORES) {
            sscanf(line, "%d", &scores[count]);
            count++;
        }
    }

    f_close(&file);

    // 3. Add new score
    scores[count++] = score;

    // 4. Sort scores (highest first)
    sort_scores(scores, count);

    // 5. Check if new score made top 10
    for (int i = 0; i < count && i < MAX_SCORES; i++) {
        if (scores[i] == score) {
            *made_top10 = true;
            break;
        }
    }

    // 6. Rewrite file with top 10
    fr = f_open(&file, FILE_NAME, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) return fr;

    for (int i = 0; i < count && i < MAX_SCORES; i++) {
        sprintf(line, "%d\n", scores[i]);
        f_puts(line, &file);
    }

    f_close(&file);

    return FR_OK;
}