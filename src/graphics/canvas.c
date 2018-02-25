#include "graphics/canvas.h"
#include "graphics/graphics.h"
#include "math/mat4.h"
#include <math.h>
#include <stdlib.h>

bool lovrCanvasSupportsFormat(TextureFormat format) {
  switch (format) {
    case FORMAT_RGB:
    case FORMAT_RGBA:
    case FORMAT_RGBA16F:
    case FORMAT_RGBA32F:
    case FORMAT_RG11B10F:
      return true;
    case FORMAT_DXT1:
    case FORMAT_DXT3:
    case FORMAT_DXT5:
      return false;
  }
}

Canvas* lovrCanvasCreate(int width, int height, TextureFormat format, int msaa, bool depth, bool stencil, bool multiview) {
  TextureData* textureData = lovrTextureDataGetEmpty(width, height, format);
  Texture* texture = lovrTextureCreate(TEXTURE_2D, &textureData, 1, true, false);

  if (!texture) return NULL;

  Canvas* canvas = lovrAlloc(sizeof(Canvas), lovrCanvasDestroy);
  canvas->texture = *texture;
  canvas->msaa = msaa;
  canvas->multiview = multiview;
  canvas->framebuffer = 0;
  canvas->eyeFramebuffers[0] = 0;
  canvas->eyeFramebuffers[1] = 0;
  canvas->resolveFramebuffer = 0;
  canvas->depthStencilRenderbuffer = 0;
  canvas->msaaTarget = 0;

  // Framebuffer
  glGenFramebuffers(1, &canvas->framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, canvas->framebuffer);

  // Color attachment
  if (multiview) {
    glGenTextures(1, &canvas->msaaTarget);
    glBindTexture(GL_TEXTURE_2D_ARRAY, canvas->msaaTarget);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, lovrTextureFormatGetGLInternalFormat(format, true), width, height, 2);
    glFramebufferTextureMultiviewOVR(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, canvas->msaaTarget, 0, 0, 2);
  } else {
    if (msaa > 0) {
      GLenum internalFormat = lovrTextureFormatGetGLInternalFormat(format, lovrGraphicsIsGammaCorrect());
      glGenRenderbuffers(1, &canvas->msaaTarget);
      glBindRenderbuffer(GL_RENDERBUFFER, canvas->msaaTarget);
      glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa, internalFormat, width, height);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, canvas->msaaTarget);
    } else {
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, canvas->texture.id, 0);
    }
  }

  // Depth/Stencil
  if (depth || stencil) {
    GLenum depthStencilFormat = stencil ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT24;
    glGenRenderbuffers(1, &canvas->depthStencilRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, canvas->depthStencilRenderbuffer);
    if (msaa > 0) {
      glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa, depthStencilFormat, width, height);
    } else {
      glRenderbufferStorage(GL_RENDERBUFFER, depthStencilFormat, width, height);
    }

    if (depth) {
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, canvas->depthStencilRenderbuffer);
    }

    if (stencil) {
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, canvas->depthStencilRenderbuffer);
    }
  }

  // Resolve
  if (msaa > 0) {
    if (multiview) {
      glGenFramebuffers(2, &canvas->eyeFramebuffers[0]);
      for (int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, canvas->eyeFramebuffers[i]);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, canvas->msaaTarget, 0, i);
      }
    }

    glGenFramebuffers(1, &canvas->resolveFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, canvas->resolveFramebuffer);
    glBindTexture(GL_TEXTURE_2D, canvas->texture.id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, canvas->texture.id, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, canvas->framebuffer);
  }

  lovrAssert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Error creating Canvas");
  lovrGraphicsClear(true, true, true, (Color) { 0, 0, 0, 0 }, 1., 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return canvas;
}

void lovrCanvasDestroy(const Ref* ref) {
  Canvas* canvas = (Canvas*) containerof(ref, Texture);
  glDeleteFramebuffers(1, &canvas->framebuffer);
  if (canvas->resolveFramebuffer) {
    glDeleteFramebuffers(1, &canvas->resolveFramebuffer);
  }
  if (canvas->depthStencilRenderbuffer) {
    glDeleteRenderbuffers(1, &canvas->depthStencilRenderbuffer);
  }
  if (canvas->msaaTarget) {
    glDeleteTextures(1, &canvas->msaaTarget);
  }
  lovrTextureDestroy(ref);
}

void lovrCanvasResolve(Canvas* canvas, int layer) {
  int width = canvas->texture.width;
  int height = canvas->texture.height;

  if (canvas->multiview) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, canvas->eyeFramebuffers[layer]);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, canvas->resolveFramebuffer);
  } else {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, canvas->framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, canvas->resolveFramebuffer);
  }

  glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

TextureFormat lovrCanvasGetFormat(Canvas* canvas) {
  return canvas->texture.slices[0]->format;
}

int lovrCanvasGetMSAA(Canvas* canvas) {
  return canvas->msaa;
}
