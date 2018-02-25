#include "graphics/texture.h"
#include "util.h"

#pragma once

typedef struct {
  Texture texture;
  GLuint framebuffer;
  GLuint eyeFramebuffers[2];
  GLuint resolveFramebuffer;
  GLuint depthStencilRenderbuffer;
  GLuint msaaTarget;
  int msaa;
  bool multiview;
} Canvas;

bool lovrCanvasSupportsFormat(TextureFormat format);

Canvas* lovrCanvasCreate(int width, int height, TextureFormat format, int msaa, bool depth, bool stencil, bool multiview);
void lovrCanvasDestroy(const Ref* ref);
void lovrCanvasResolve(Canvas* canvas, int layer);
TextureFormat lovrCanvasGetFormat(Canvas* canvas);
int lovrCanvasGetMSAA(Canvas* canvas);
