#pragma once
#ifndef TEXTURE_HPP
#define TEXTURE_HPP
#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <vector>

/**
 *  Wrapper class for the stbi texture data with helper methods.
 *  DO NOT attempt to free it yourself (if you need to free a texture before end
 * of execution use the freeTexture method)
 */
struct Texture {
  unsigned char *data;
  int width, height, channels;
  GLuint openglimg;
  bool isHDR = false;
  bool loadedToGPU = false;
  Texture() = default;
  Texture(const Texture &) = delete;
  Texture(Texture &&other) = delete;
  Texture &operator=(const Texture &) = delete;
  Texture &operator=(Texture &&) = delete;
  Texture(GLuint id, int width, int height)
      : openglimg(id), width(width), height(height) {}
  ~Texture();
  /**
   * Resizes the given opengl texture data with the given parameters.
   * @param tex the opengl texture id
   * @param type the opengl internal format
   * @param datatype the data type
   * @param format the opengl color format
   */
  static void resizeTexture(GLuint tex, unsigned int width, unsigned int height,
                            GLuint type, GLuint datatype = GL_FLOAT,
                            GLuint format = GL_RGBA);
  static GLuint loadBinary(unsigned char *data, int width, int height,
                           int channels, GLint wrap = GL_REPEAT,
                           GLint minFilter = GL_LINEAR,
                           GLint magFilter = GL_LINEAR);
  static GLuint loadBinary(float *data, int width, int height, int channels,
                           GLint wrap = GL_REPEAT, GLint minFilter = GL_LINEAR,
                           GLint magFilter = GL_LINEAR);
  static GLuint loadBinary3D(unsigned char *data, int width, int height,
                             int depth, int channels, GLint wrap = GL_REPEAT,
                             GLint minFilter = GL_LINEAR,
                             GLint magFilter = GL_LINEAR);
  static GLuint loadBinary3D(float *data, int width, int height, int depth,
                             int channels, GLint wrap = GL_REPEAT,
                             GLint minFilter = GL_LINEAR,
                             GLint magFilter = GL_LINEAR);
  static void enableTextureMipMapping(GLuint tex);
  /**
   * Loads the image data to the GPU if not already present.
   * When calling getTexture with keepData=false it is already loaded to the
   * gpu.
   */
  void loadToGPU(GLint wrap = GL_REPEAT, GLint minFilter = GL_LINEAR,
                 GLint magFilter = GL_LINEAR);

  void bind() { glBindTexture(GL_TEXTURE_2D, openglimg); }

  void unbind() { glBindTexture(GL_TEXTURE_2D, 0); }
  void enableMipMapping();
};
/**
 *  Finds the texture loaded by the given path.
 *  if it was not loaded before it is then loaded and associated with this path.
 *  channels are the desired channels per pixel (default 1 = 8bit, 0-255 color
 * space), only relevant if the texture is referenced the first time (i.e. if it
 * has to be loaded) returns nullptr on error. Do not attempt to delete the
 * object by yourself, use the freeTexture method.
 *  @param path path to the texture
 *  @param keepData set this to true if you want to access the data on the cpu
 * after it has been loaded to the gpu
 */
Texture *getTextureFile(std::string path, bool keepData = false);
void freeTextureFile(std::string path);

#endif
