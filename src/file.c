#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "ff.h"
#include "diskio.h"
#include "pico/stdlib.h"
#include "file.h"

#define LEADERBOARD_FILE   "0:/scores.csv"
#define LINE_BUF_LEN       128

ScoreEntry leaderboard[MAX_ENTRIES];
int leaderboard_count = 0;

static void trim_newline(char *s) {
    if (s == NULL) {
        return;
    }

    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        len--;
    }
}

static void trim_spaces(char *s) {
    if (s == NULL) {
        return;
    }

    char *start = s;
    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }

    if (start != s) {
        memmove(s, start, strlen(start) + 1);
    }

    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[len - 1] = '\0';
        len--;
    }
}

static void sanitize_username(char *username) {
    if (username == NULL) {
        return;
    }

    trim_newline(username);
    trim_spaces(username);

    size_t write_idx = 0;
    for (size_t read_idx = 0; username[read_idx] != '\0' && write_idx < MAX_USERNAME_LEN; read_idx++) {
        unsigned char c = (unsigned char)username[read_idx];
        if (c == ',') {
            username[write_idx++] = '_';
        } else if (isprint(c)) {
            username[write_idx++] = (char)c;
        }
    }
    username[write_idx] = '\0';
}

static void clear_leaderboard(void) {
    leaderboard_count = 0;
    memset(leaderboard, 0, sizeof(leaderboard));
}

static void sort_leaderboard(void) {
    for (int i = 0; i < leaderboard_count - 1; i++) {
        for (int j = i + 1; j < leaderboard_count; j++) {
            if (leaderboard[j].score > leaderboard[i].score) {
                ScoreEntry temp = leaderboard[i];
                leaderboard[i] = leaderboard[j];
                leaderboard[j] = temp;
            }
        }
    }
}

static int find_existing_user(const char *username) {
    for (int i = 0; i < leaderboard_count; i++) {
        if (strcmp(leaderboard[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

static int lowest_score(void) {
    if (leaderboard_count == 0) {
        return -1;
    }
    return leaderboard[leaderboard_count - 1].score;
}

static bool score_qualifies(int score) {
    if (leaderboard_count < MAX_ENTRIES) {
        return true;
    }
    return score > lowest_score();
}

FRESULT leaderboard_create_default(void) {
    FIL file;
    UINT bw = 0;
    const char *header = "Rank,Username,Score\r\n";

    FRESULT fr = f_open(&file, LEADERBOARD_FILE, FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        return fr;
    }

    fr = f_write(&file, header, (UINT)strlen(header), &bw);
    if (fr == FR_OK && bw != strlen(header)) {
        fr = FR_DISK_ERR;
    }

    f_close(&file);
    return fr;
}

FRESULT leaderboard_load(void) {
    FIL file;
    char line[LINE_BUF_LEN];

    clear_leaderboard();

    FRESULT fr = f_open(&file, LEADERBOARD_FILE, FA_READ);
    if (fr == FR_NO_FILE) {
        return leaderboard_create_default();
    }
    if (fr != FR_OK) {
        return fr;
    }

    if (f_gets(line, sizeof(line), &file) == NULL) {
        f_close(&file);
        return FR_OK;
    }

    while (leaderboard_count < MAX_ENTRIES && f_gets(line, sizeof(line), &file) != NULL) {
        trim_newline(line);
        if (line[0] == '\0') {
            continue;
        }

        char *rank = strtok(line, ",");
        char *username = strtok(NULL, ",");
        char *score_str = strtok(NULL, ",");
        (void)rank;

        if (username == NULL || score_str == NULL) {
            continue;
        }

        sanitize_username(username);
        if (username[0] == '\0') {
            continue;
        }

        strncpy(leaderboard[leaderboard_count].username, username, MAX_USERNAME_LEN);
        leaderboard[leaderboard_count].username[MAX_USERNAME_LEN] = '\0';
        leaderboard[leaderboard_count].score = atoi(score_str);
        leaderboard_count++;
    }

    f_close(&file);
    sort_leaderboard();
    return FR_OK;
}

FRESULT leaderboard_save(void) {
    FIL file;
    UINT bw = 0;
    char line[LINE_BUF_LEN];
    const char *header = "Rank,Username,Score\r\n";

    FRESULT fr = f_open(&file, LEADERBOARD_FILE, FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        return fr;
    }

    fr = f_write(&file, header, (UINT)strlen(header), &bw);
    if (fr != FR_OK || bw != strlen(header)) {
        f_close(&file);
        return (fr == FR_OK) ? FR_DISK_ERR : fr;
    }

    for (int i = 0; i < leaderboard_count; i++) {
        int len = snprintf(line, sizeof(line), "%d,%s,%d\r\n",
                           i + 1,
                           leaderboard[i].username,
                           leaderboard[i].score);
        if (len < 0 || len >= (int)sizeof(line)) {
            f_close(&file);
            return FR_INVALID_OBJECT;
        }

        fr = f_write(&file, line, (UINT)len, &bw);
        if (fr != FR_OK || bw != (UINT)len) {
            f_close(&file);
            return (fr == FR_OK) ? FR_DISK_ERR : fr;
        }
    }

    fr = f_sync(&file);
    f_close(&file);
    return fr;
}

bool leaderboard_add_or_update(const char *username, int new_score) {
    if (username == NULL || username[0] == '\0') {
        return false;
    }

    if (new_score < 0) {
        return false;
    }

    if (!score_qualifies(new_score)) {
        return false;
    }

    if (leaderboard_count < MAX_ENTRIES) {
        strncpy(leaderboard[leaderboard_count].username, username, MAX_USERNAME_LEN);
        leaderboard[leaderboard_count].username[MAX_USERNAME_LEN] = '\0';
        leaderboard[leaderboard_count].score = new_score;
        leaderboard_count++;
    } else {
        strncpy(leaderboard[leaderboard_count - 1].username, username, MAX_USERNAME_LEN);
        leaderboard[leaderboard_count - 1].username[MAX_USERNAME_LEN] = '\0';
        leaderboard[leaderboard_count - 1].score = new_score;
    }

    sort_leaderboard();
    return true;
}

void leaderboard_print(void) {
    printf("\n=== LEADERBOARD ===\n");
    if (leaderboard_count == 0) {
        printf("No scores yet.\n");
        return;
    }

    for (int i = 0; i < leaderboard_count; i++) {
        printf("%2d. %-20s %d\n", i + 1, leaderboard[i].username, leaderboard[i].score);
    }
}

FRESULT leaderboard_submit_score(const char *username, int score, bool *made_top10) {
    FRESULT fr;
    char username_buf[MAX_USERNAME_LEN + 1];

    if (made_top10 != NULL) {
        *made_top10 = false;
    }

    if (username == NULL) {
        return FR_INVALID_NAME;
    }

    strncpy(username_buf, username, MAX_USERNAME_LEN);
    username_buf[MAX_USERNAME_LEN] = '\0';

    fr = leaderboard_load();
    if (fr != FR_OK) {
        return fr;
    }

    if (!score_qualifies(score) && leaderboard_count >= MAX_ENTRIES) {
        printf("Score %d did not make the top %d.\n", score, MAX_ENTRIES);
        return FR_OK;
    }

    sanitize_username(username_buf);
    if (username_buf[0] == '\0') {
        return FR_INVALID_NAME;
    }

    if (!leaderboard_add_or_update(username_buf, score)) {
        printf("Score was not added. Existing score may already be higher.\n");
        return FR_OK;
    }

    fr = leaderboard_save();
    if (fr != FR_OK) {
        return fr;
    }

    if (made_top10 != NULL) {
        *made_top10 = true;
    }

    leaderboard_print();
    return FR_OK;
}

void test_leaderboard(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: testlb <username> <score>\n");
        return;
    }

    char username[MAX_USERNAME_LEN + 1];
    strncpy(username, argv[1], MAX_USERNAME_LEN);
    username[MAX_USERNAME_LEN] = '\0';
    sanitize_username(username);

    int score = atoi(argv[2]);
    if (score < 0) {
        printf("Score must be non-negative.\n");
        return;
    }

    FRESULT fr = leaderboard_load();
    if (fr != FR_OK) {
        printf("Load failed: %d\n", fr);
        return;
    }

    if (leaderboard_add_or_update(username, score)) {
        fr = leaderboard_save();
        if (fr != FR_OK) {
            printf("Save failed: %d\n", fr);
            return;
        }
        printf("Added/updated score for %s.\n", username);
    } else {
        printf("Score did not change leaderboard.\n");
    }

    leaderboard_print();
}
