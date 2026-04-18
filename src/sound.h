#ifndef SOUND_H
#define SOUND_H

void sound_init(void);
void sound_play_start(void);   // blocking ascending jingle on game start
void sound_play_tick(void);    // short blip on each scroll tick
void sound_play_death(void);   // blocking descending tone on game over

#endif
