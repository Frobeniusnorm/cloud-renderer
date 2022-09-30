#include "texture.hpp"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
Texture::~Texture() {
  if (data)
    stbi_image_free(data);
  if (loadedToGPU)
    glDeleteTextures(1, &openglimg);
}

struct MemoryWrapper {
  std::unordered_map<std::string, Texture *> cont;
  ~MemoryWrapper() {
    for (auto p : cont)
      delete p.second;
  }
};
static MemoryWrapper textures;

Texture *getTexture(std::string path, bool keepData) {
  if (textures.cont.find(path) == textures.cont.end()) {
    // load texture first
    Texture *tex = new Texture();
    stbi_set_flip_vertically_on_load(true);
    tex->isHDR = stbi_is_hdr(path.c_str());
    if (tex->isHDR)
      tex->data = (uint8_t *)stbi_loadf(path.c_str(), &tex->height, &tex->width,
                                        &tex->channels, 0);
    else
      tex->data =
          stbi_load(path.c_str(), &tex->width, &tex->height, &tex->channels, 0);
    if (!tex->data) {
      std::cerr << "Could not load Image \"" << path << "\"" << std::endl;
      delete tex;
      return nullptr;
    }
    if (!keepData) {
      tex->loadToGPU();
      stbi_image_free(tex->data);
      tex->data = nullptr;
    }
    textures.cont.insert({path, tex});
  }
  return textures.cont[path];
}
void freeTexture(std::string path) {
  if (textures.cont.find(path) != textures.cont.end())
    delete textures.cont[path];
}
void Texture::resizeTexture(GLuint tex, unsigned int width, unsigned int height,
                            GLuint type, GLuint datatype, GLuint format) {
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, type, width, height, 0, format, datatype, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
}
void Texture::loadToGPU(GLint wrap, GLint minFilter, GLint magFilter) {
  if (!loadedToGPU) {

    glGenTextures(1, &openglimg);
    glBindTexture(GL_TEXTURE_2D, openglimg);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    if (isHDR) {
      glTexImage2D(GL_TEXTURE_2D, 0,
                   channels == 4   ? GL_RGBA32F
                   : channels == 3 ? GL_RGB32F
                   : channels == 2 ? GL_RG32F
                                   : GL_R32F,
                   width, height, 0,
                   channels == 4   ? GL_RGBA
                   : channels == 3 ? GL_RGB
                   : channels == 2 ? GL_RG
                                   : GL_RED,
                   GL_FLOAT, &data[0]);
    } else {
      glTexImage2D(GL_TEXTURE_2D, 0,
                   channels == 4   ? GL_RGBA8
                   : channels == 3 ? GL_RGB8
                   : channels == 2 ? GL_RG8
                                   : GL_R8,
                   width, height, 0,
                   channels == 4   ? GL_RGBA
                   : channels == 3 ? GL_RGB
                   : channels == 2 ? GL_RG
                                   : GL_RED,
                   GL_UNSIGNED_BYTE, &data[0]);
    }
    loadedToGPU = true;
  }
}
void Texture::enableMipMapping() { enableTextureMipMapping(openglimg); }

GLuint Texture::loadBinary(unsigned char *data, int width, int height,
                           int channels, GLint wrap, GLint minFilter,
                           GLint magFilter) {
  GLuint foo;
  glGenTextures(1, &foo);
  glBindTexture(GL_TEXTURE_2D, foo);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
  glTexImage2D(GL_TEXTURE_2D, 0,
               channels == 4   ? GL_RGBA32F
               : channels == 3 ? GL_RGB32F
               : channels == 2 ? GL_RG32F
                               : GL_R32F,
               width, height, 0,
               channels == 4   ? GL_RGBA
               : channels == 3 ? GL_RGB
               : channels == 2 ? GL_RG
                               : GL_RED,
               GL_UNSIGNED_BYTE, &data[0]);

  return foo;
}
GLuint Texture::loadBinary(float *data, int width, int height, int channels,
                           GLint wrap, GLint minFilter, GLint magFilter) {
  GLuint foo;
  glGenTextures(1, &foo);
  glBindTexture(GL_TEXTURE_2D, foo);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
  glTexImage2D(GL_TEXTURE_2D, 0,
               channels == 4   ? GL_RGBA32F
               : channels == 3 ? GL_RGB32F
               : channels == 2 ? GL_RG32F
                               : GL_R32F,
               width, height, 0,
               channels == 4   ? GL_RGBA
               : channels == 3 ? GL_RGB
               : channels == 2 ? GL_RG
                               : GL_RED,
               GL_FLOAT, &data[0]);
  return foo;
}
GLuint Texture::loadBinary3D(unsigned char *data, int width, int height,
                             int depth, int channels, GLint wrap,
                             GLint minFilter, GLint magFilter) {
  GLuint foo;
  glGenTextures(1, &foo);
  glBindTexture(GL_TEXTURE_3D, foo);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, magFilter);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, minFilter);
  glTexImage3D(GL_TEXTURE_3D, 0,
               channels == 4   ? GL_RGBA32F
               : channels == 3 ? GL_RGB32F
               : channels == 2 ? GL_RG32F
                               : GL_R32F,
               width, height, depth, 0,
               channels == 4   ? GL_RGBA
               : channels == 3 ? GL_RGB
               : channels == 2 ? GL_RG
                               : GL_RED,
               GL_UNSIGNED_BYTE, &data[0]);

  return foo;
}
GLuint Texture::loadBinary3D(float *data, int width, int height, int depth,
                             int channels, GLint wrap, GLint minFilter,
                             GLint magFilter) {
  GLuint foo;
  glGenTextures(1, &foo);
  glBindTexture(GL_TEXTURE_3D, foo);

  glTexImage3D(GL_TEXTURE_3D, 0,
               channels == 4   ? GL_RGBA32F
               : channels == 3 ? GL_RGB32F
               : channels == 2 ? GL_RG32F
                               : GL_R32F,
               width, height, depth, 0,
               channels == 4   ? GL_RGBA
               : channels == 3 ? GL_RGB
               : channels == 2 ? GL_RG
                               : GL_RED,
               GL_FLOAT, &data[0]);
  return foo;
}

void Texture::enableTextureMipMapping(GLuint tex) {
  glBindTexture(GL_TEXTURE_2D, tex);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
