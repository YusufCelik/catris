#ifndef ANIMATION_H
#define ANIMATION_H

#include "config.h"

typedef struct SingleBlock SingleBlock;
typedef struct GameState GameState;

typedef struct {
	float startValue;
	float endValue;
	float currentValue;
	float stepValue;
} AnimationProperty;

typedef void(*AnimationStepCallback)(GameState* gameState, SingleBlock **animation_objects, size_t *num_animation_objects, AnimationProperty* properties, size_t numProperties);
typedef void(*AnimationCompleteCallback)(GameState* gameState, SingleBlock **animation_objects, size_t *num_animation_objects, AnimationProperty* properties, size_t numProperties);

typedef enum {
	ANIM_LINEAR,
	ANIM_EASE_OUT_ELASTIC,
	ANIM_EASE_OUT_BOUNCE
} AnimationType;

typedef struct {
	double startTime;
	double duration;
	AnimationProperty* properties;
	size_t numProperties;
	AnimationStepCallback stepCallback;
	AnimationCompleteCallback completeCallback;
	SingleBlock* animation_objects[GRID_SURFFACE];
	size_t num_animation_objects;
	AnimationType type;
} Animation;

typedef struct {
	Animation rowDownwardsAnimation;
	Animation rowDestructionAnimation;
} Animations;

float easeOutElastic(float progress);
float easeOutBounce(float progress);
void updateAnimation(Animation* animation, GameState* gameState, double currentTime);

#endif // ANIMATION_H