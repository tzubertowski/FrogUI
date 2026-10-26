#ifndef FROGUI_PILL_ANIMATION_H
#define FROGUI_PILL_ANIMATION_H

typedef struct {
	int from_y, to_y, current_y;
	int frame, frames;
} pill_animation;

static inline void pill_animation_reset(pill_animation *animation, int y)
{
	animation->from_y = animation->to_y = animation->current_y = y;
	animation->frame = animation->frames = 0;
}

static inline void pill_animation_start(pill_animation *animation,
		int from_y, int to_y, int frames)
{
	if (frames <= 0 || from_y == to_y) {
		pill_animation_reset(animation, to_y);
		return;
	}
	animation->from_y = animation->current_y = from_y;
	animation->to_y = to_y;
	animation->frame = 0;
	animation->frames = frames;
}

static inline int pill_animation_step(pill_animation *animation)
{
	if (animation->frame < animation->frames) {
		float p, ease;
		p = (float)++animation->frame / animation->frames;
		ease = p * p * (3.0f - 2.0f * p);
		animation->current_y = animation->from_y +
			(int)((animation->to_y - animation->from_y) * ease);
		if (animation->frame == animation->frames)
			animation->current_y = animation->to_y;
	}
	return animation->current_y;
}

static inline int pill_animation_active(const pill_animation *animation)
{
	return animation->frame < animation->frames;
}

#endif
