#ifndef FILE_H
#define FILE_H

#include "ff.h"

#define MAX_ENTRIES        5
#define MAX_USERNAME_LEN   20

typedef struct {
    char username[MAX_USERNAME_LEN + 1];
    int score;
} ScoreEntry;

extern ScoreEntry leaderboard[MAX_ENTRIES];
extern int leaderboard_count;

FRESULT leaderboard_load(void);
FRESULT leaderboard_save(void);
FRESULT leaderboard_submit_score(int score, bool *made_top10);
bool leaderboard_add_or_update(const char *username, int new_score);
void leaderboard_print(void);
void test_leaderboard(int argc, char *argv[]);

#endif