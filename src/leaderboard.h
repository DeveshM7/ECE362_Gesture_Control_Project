#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#include "ff.h"
#include <stdbool.h>

FRESULT leaderboard_submit_score(int score, bool *made_top10);

#endif