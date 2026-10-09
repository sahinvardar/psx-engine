#ifndef GAME_TIME_H
#define GAME_TIME_H

#include <stdint.h>

#define TIME_ONE_SECOND 65536
#define TIME_SECONDS(value) ((TimeDelta) (value) * TIME_ONE_SECOND)

typedef int32_t TimeDelta;

typedef struct {
	uint32_t previous_vblank;
	uint32_t total_vblanks;
	TimeDelta delta;
	uint16_t refresh_rate;
} GameTime;

void game_time_init(GameTime *time);
void game_time_update(GameTime *time);
int32_t time_scale_rate(int32_t units_per_second, TimeDelta delta);

#endif
