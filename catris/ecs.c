#include "ecs.h"

void initECS(ECS* ecs) {
	ecs->numAvailable = MAX_ENTITIES;
	for (unsigned int i = 0; i < MAX_ENTITIES; ++i) {
		ecs->availableIDs[i] = MAX_ENTITIES - i - 1;
		ecs->entities[i].id = MAX_ENTITIES; // Invalid ID to indicate unused entity slot
		ecs->entities[i].componentMask = COMPONENT_NONE;
	}
}

EntityID createEntity(ECS* ecs) {
	if (ecs->numAvailable > 0) {
		EntityID id = ecs->availableIDs[--ecs->numAvailable];
		ecs->entities[id].id = id;
		ecs->entities[id].componentMask = COMPONENT_NONE;
		return id;
	}
	return MAX_ENTITIES; // No available ID
}

void destroyEntity(ECS* ecs, EntityID id) {
	ecs->entities[id].componentMask = COMPONENT_NONE;
	ecs->availableIDs[ecs->numAvailable++] = id;
}