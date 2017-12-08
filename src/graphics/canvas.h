#include "graphics/texture.h"
#include "util.h"

typedef enum {
  CANVAS_3D,
  CANVAS_2D
} CanvasType;

typedef struct {
  Texture texture;
  CanvasType type;
  GLuint framebuffer;
  GLuint resolveFramebuffer;
  GLuint depthBuffer;
  GLuint msaaTexture;
  int msaa;
} Canvas;

Canvas* lovrCanvasCreate(int width, int height, CanvasType type, int msaa);
void lovrCanvasDestroy(const Ref* ref);
void lovrCanvasBind(Canvas* canvas);
void lovrCanvasResolveMSAA(Canvas* canvas);
int lovrCanvasGetMSAA(Canvas* canvas);
