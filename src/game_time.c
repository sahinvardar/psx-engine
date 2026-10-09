#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <psxgpu.h>

#include "game_time.h"

void game_time_init(GameTime *time) {
	assert(time != NULL);
	time->previous_vblank = (uint32_t) VSync(-1);
	time->total_vblanks = 0;
	time->delta = 0;
	time->refresh_rate = GetVideoMode() == MODE_PAL ? 50 : 60;
}

void game_time_update(GameTime *time) {
	uint32_t current_vblank;
	uint32_t elapsed_vblanks;
	int64_t scaled_delta;

	assert(time != NULL);
	assert(time->refresh_rate > 0);

	current_vblank = (uint32_t) VSync(-1);
	elapsed_vblanks = current_vblank - time->previous_vblank;
	time->previous_vblank = current_vblank;
	time->total_vblanks += elapsed_vblanks;

	scaled_delta =
		(int64_t) elapsed_vblanks * TIME_ONE_SECOND
		/ time->refresh_rate;
	assert(scaled_delta <= INT32_MAX);
	time->delta = (TimeDelta) scaled_delta;
}

int32_t time_scale_rate(int32_t units_per_second, TimeDelta delta) {
	int64_t scaled = (int64_t) units_per_second * delta;

	if (scaled >= 0) {
		scaled += TIME_ONE_SECOND / 2;
	} else {
		scaled -= TIME_ONE_SECOND / 2;
	}
	scaled /= TIME_ONE_SECOND;

	assert(scaled >= INT32_MIN && scaled <= INT32_MAX);
	return (int32_t) scaled;
}
