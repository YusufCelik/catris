#ifndef SYSTEM_ACTIONS_H
#define SYSTEM_ACTIONS_H

typedef enum {
	IDLE,
	PLAYER_FINISHED_MOVE,
	CHECK_ROW_COMPLETION,
	DESTROY_ROW,
	ROW_DESTROYED,
	SPAWN_NEXT_BLOCK,
	START_ROW_DESCENT_ANIMATION,
	START_ROW_REMOVAL_ANIMATION
} SystemActions;

#endif