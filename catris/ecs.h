#ifndef ECS_H
#define ECS_H

#include <cglm/struct.h>

#define MAX_ENTITIES 1000

typedef struct {
	float dx, dy;
} VelocityComponent;

typedef struct {
	unsigned int VAO;
	unsigned int VBO;
	unsigned int VEO;
} OpenglComponent;

typedef struct {
	float x;
	float y;
} TileComponent;

typedef struct {
	mat4 model;
} ModelComponent;

typedef struct {
	float right;
	float left;
	float top;
	float bottom;
	float near;
	float far;
} CameraComponent;

typedef struct {
	float vertices[32];
	unsigned int indices[6];
} VertexComponent;

typedef struct {
	char* path;
	unsigned int textureId;
} TextureComponent;

typedef unsigned int EntityID;
typedef unsigned int ComponentMask;

// Component bitmasks
#define COMPONENT_NONE     0
#define COMPONENT_MODEL    1  // binary: 0001
#define COMPONENT_VELOCITY 2  // binary: 0010
#define COMPONENT_OPENGL   4  // binary: 0100
#define COMPONENT_TILE     8  // binary: 1000
#define COMPONENT_CAMERA   16 // binary: 0001 0000
#define COMPONENT_VERTEX   32 // binary: 0010 0000
#define COMPONENT_TEXTURE  64 // binary: 0100 0000

typedef struct {
	EntityID id;
	ComponentMask componentMask;
} Entity;

typedef struct {
	Entity entities[MAX_ENTITIES];
	ModelComponent modelComponent[MAX_ENTITIES];
	VelocityComponent velocityComponents[MAX_ENTITIES];
	OpenglComponent openglComponents[MAX_ENTITIES];
	TileComponent tileComponents[MAX_ENTITIES];
	CameraComponent cameraComponents[MAX_ENTITIES];
	VertexComponent vertexComponents[MAX_ENTITIES];
	TextureComponent textureComponents[MAX_ENTITIES];
	unsigned int availableIDs[MAX_ENTITIES];
	unsigned int numAvailable;
} ECS;

void initECS(ECS* ecs);

EntityID createEntity(ECS* ecs);

void destroyEntity(ECS* ecs, EntityID id);

#endif