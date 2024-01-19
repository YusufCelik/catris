#include "config.h"
#include "ecs.h"
#include "app_context.h"

EntityID createCamera(ECS *ecs) {
	unsigned int cameraId = createEntity(ecs);
	CameraComponent cameraComponent;
	cameraComponent.right = SCREEN_WIDTH / 2;
	cameraComponent.left = -cameraComponent.right;
	cameraComponent.top = SCREEN_HEIGHT / 2;
	cameraComponent.bottom = -cameraComponent.top;
	cameraComponent.near = -1.0f;
	cameraComponent.far = 1.0f;

	ModelComponent cameraModel;
	glm_ortho(cameraComponent.left, cameraComponent.right, cameraComponent.bottom, cameraComponent.top, 
		cameraComponent.near, cameraComponent.far, cameraModel.model);

	ecs->cameraComponents[cameraId] = cameraComponent;
	ecs->modelComponent[cameraId] = cameraModel; 
	ecs->entities[cameraId].componentMask |= COMPONENT_CAMERA;
	ecs->entities[cameraId].componentMask |= COMPONENT_MODEL;

	printf("%d entity", cameraId);
	return cameraId;
}

void setActiveCamera(unsigned cameraId, ApplicationContext *context) {
	context->activeCameraId = cameraId;
}