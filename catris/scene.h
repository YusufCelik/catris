typedef struct Scene Scene; // Forward declaration

typedef enum {
	SCENE_ACTIVE,
	SCENE_TRANSITION_IN,
	SCENE_TRANSITION_OUT,
	SCENE_INACTIVE
} SceneState;

struct Scene {
	void(*init)(Scene *self);
	void(*update)(Scene *self, double deltaTime);
	void(*draw)(Scene *self);
	void(*destroy)(Scene *self);
	SceneState state;
};
