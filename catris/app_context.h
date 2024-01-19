#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "ecs.h"
#include "scene.h"
#include "hash.h"

typedef struct {
	unsigned int programID;
} ShaderManager;

typedef struct {
	Scene *currentScene;
} SceneManager;

typedef struct {
	ht_hash_table *textureIds;
	ht_hash_table *textureWidths;
	ht_hash_table *textureHeights;
} TextureManager;

typedef struct {
	ShaderManager* shaderManager;
	SceneManager* sceneManager;
	TextureManager* textureManager;
	EntityID activeCameraId;
} ApplicationContext;

char* readShaderSource(const char* filePath);
void load_textures(ApplicationContext *context);
unsigned int get_texture_id_from_path(ApplicationContext * context, char *path);
void get_texture_dimensions_from_id(ApplicationContext * context, char *path, unsigned int *dimensions);
#endif