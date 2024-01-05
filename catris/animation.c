#define _USE_MATH_DEFINES
#include <math.h>
#include "animation.h"

float easeOutElastic(float progress) {
	// Simplified implementation of easeOutElastic
	float p = 0.3f;
	return pow(2, -10 * progress) * sin((progress - p / 4) * (2 * M_PI) / p) + 1;
}

float easeOutBounce(float progress) {
	// Simplified implementation of easeOutBounce
	if (progress < (1 / 2.75f)) {
		return 7.5625f * progress * progress;
	}
	else if (progress < (2 / 2.75f)) {
		progress -= 1.5f / 2.75f;
		return 7.5625f * progress * progress + 0.75f;
	}
	else if (progress < (2.5 / 2.75)) {
		progress -= 2.25f / 2.75f;
		return 7.5625f * progress * progress + 0.9375f;
	}
	else {
		progress -= 2.625f / 2.75f;
		return 7.5625f * progress * progress + 0.984375f;
	}
}

void updateAnimation(Animation* animation, GameState* gameState, double currentTime) {
	if (!animation || !gameState) return;

	double elapsed = currentTime - animation->startTime;
	if (elapsed < animation->duration) {
		float progress = elapsed / animation->duration;

		// Apply the easing function based on the animation type
		switch (animation->type) {
		case ANIM_EASE_OUT_ELASTIC:
			progress = easeOutElastic(progress);
			break;
		case ANIM_EASE_OUT_BOUNCE:
			progress = easeOutBounce(progress);
			break;
		case ANIM_LINEAR:
		default:
			// Linear progress, no change needed
			break;
		}

		for (size_t i = 0; i < animation->numProperties; i++) {
			float nextValue = animation->properties[i].startValue +
				(animation->properties[i].endValue - animation->properties[i].startValue) * progress;

			// Calculate the step value for this frame
			animation->properties[i].stepValue = nextValue - animation->properties[i].currentValue;
			animation->properties[i].currentValue = nextValue;

			printf("Elapsed: %f, Progress: %f, NextValue: %f, StepValue: %f, CurrentValue: %f \n", elapsed, progress, nextValue, animation->properties[i].stepValue, animation->properties[i].currentValue);
		}


		if (animation->stepCallback) {
			animation->stepCallback(gameState, animation->animation_objects, &animation->num_animation_objects, animation->properties, animation->numProperties);
		}
	}
	else {
		// Ensure we set the exact end values and call the complete callback
		for (size_t i = 0; i < animation->numProperties; i++) {
			animation->properties[i].currentValue = animation->properties[i].endValue;
			animation->properties[i].stepValue = animation->properties[i].endValue - animation->properties[i].currentValue;
		}

		if (animation->completeCallback) {
			animation->completeCallback(gameState, animation->animation_objects, &animation->num_animation_objects, animation->properties, animation->numProperties);
		}
	}
}