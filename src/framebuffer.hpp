#pragma once
#include <optional>
#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
class Framebuffer {
  struct Attachement {
    GLuint id;
    virtual bool isTexture() = 0;
  };
  struct TextureAttachement : public Attachement {
    bool owned = false;
    GLuint type;
    virtual bool isTexture() { return true; }
  };
  struct RenderbufferAttachement : public Attachement {
    virtual bool isTexture() { return false; }
  };
  bool moved = false;
  int width = 0;
  int height = 0;
  GLuint id;
  void move(Framebuffer &foo) {
    foo.moved = true;
    id = foo.id;
    width = foo.width;
    height = foo.height;
    attachements = foo.attachements;
    targets = foo.targets;
    oldviewp[0] = foo.oldviewp[0];
    oldviewp[1] = foo.oldviewp[1];
    oldviewp[2] = foo.oldviewp[2];
    oldviewp[3] = foo.oldviewp[3];
    oldfbo = foo.oldfbo;
  }
  // to restore the viewport
  GLint oldviewp[4];
  GLint oldfbo;
  // attachements
  std::vector<GLenum> targets;
  std::vector<Attachement *> attachements;
  Attachement *depth = nullptr;
  void deleteAttachement(Attachement *atch) {
    if (atch->isTexture()) {
      TextureAttachement *ta = (TextureAttachement *)atch;
      if (ta->owned)
        glDeleteTextures(1, &(ta->id));
      delete ta;
    } else {
      RenderbufferAttachement *ra = (RenderbufferAttachement *)atch;
      glDeleteRenderbuffers(1, &(ra->id));
      delete ra;
    }
  }

public:
  /**
   *  Creates a new framebuffer with the given width and height and no
   * attachements
   */
  Framebuffer(int _width, int _height) : width(_width), height(_height) {
    glGenFramebuffers(1, &id);
  }
  ~Framebuffer() {
    if (!moved) {
      glDeleteFramebuffers(1, &id);
      if (depth)
        attachements.push_back(depth);
      for (Attachement *atch : attachements) {
        deleteAttachement(atch);
      }
    }
  }
  Framebuffer(const Framebuffer &) = delete;
  Framebuffer &operator=(const Framebuffer &) = delete;
  Framebuffer(Framebuffer &&foo) { move(foo); };
  Framebuffer &operator=(Framebuffer &&foo) {
    move(foo);
    return *this;
  };
  /**
   *  Binds the framebuffer and its attached buffers
   */
  void bind() {
    glGetIntegerv(GL_VIEWPORT, oldviewp);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldfbo);
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glDrawBuffers(GLsizei(targets.size()), targets.data());
  }
  /**
   *  Restores the state from the beginning of the previous @code{bind()} call
   */
  void unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, oldfbo);
    glViewport(oldviewp[0], oldviewp[1], oldviewp[2], oldviewp[3]);
  }
  /**
   *  Adds the given texture to this framebuffer as the depth attachement.
   *  If a previous added / generated attachement has been generated, it will be
   * deleted. This texture will NOT be automatically resized, since the internal
   * formats are unknown.
   */
  void setDepthTexture(GLuint tex) {
    if (depth)
      deleteAttachement(depth);
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    depth = new TextureAttachement();
    depth->id = tex;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
  /**
   *  Generates a new texture with the width and height of the framebuffer
   *  and adds it as the depth attachement.
   *  If a previous added / generated attachement has been generated, it will be
   * deleted.
   */
  void generateDepthTexture(GLuint depthComp = GL_DEPTH_COMPONENT16) {
    if (depth)
      deleteAttachement(depth);
    GLuint dt;
    glGenTextures(1, &dt);
    glBindTexture(GL_TEXTURE_2D, dt);
    glTexImage2D(GL_TEXTURE_2D, 0, depthComp, width, height, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    setDepthTexture(dt);
    TextureAttachement *pt = ((TextureAttachement *)depth);
    pt->owned = true;
    pt->type = depthComp;
  }
  /**
   * Generates a default Render Buffer and adds it to the depth attachement.
   * If a previous added / generated attachement has been generated, it will be
   * deleted.
   */
  void generateDepthBuffer(GLuint depthComp = GL_DEPTH_COMPONENT16) {
    if (depth)
      deleteAttachement(depth);
    GLuint rboDepthStencil;
    glGenRenderbuffers(1, &rboDepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, depthComp, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, rboDepthStencil);
    depth = new RenderbufferAttachement();
    depth->id = rboDepthStencil;
  }
  /**
   * Adds the given texture to this framebuffer as a new color attachement.
   * This texture will NOT be automatically resized, since the internal formats
   * are unknown.
   */
  void addColorTexture(GLuint tex) {
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0 + targets.size(), GL_TEXTURE_2D,
                           tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    TextureAttachement *foo = new TextureAttachement();
    foo->id = tex;
    attachements.push_back(foo);
    targets.push_back(GL_COLOR_ATTACHMENT0 + targets.size());
  }
  /**
   * Generates a texture and adds it to this framebuffer as a new color
   * attachement.
   */
  void generateColorTexture(GLint internal = GL_RGB, GLint datatype = GL_FLOAT,
                            GLuint clamptype = GL_NEAREST) {
    GLuint foo;
    glGenTextures(1, &foo);
    glBindTexture(GL_TEXTURE_2D, foo);
    glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0, GL_RGBA,
                 datatype, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, clamptype);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, clamptype);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    addColorTexture(foo);
    TextureAttachement *ptr =
        (TextureAttachement *)attachements[attachements.size() - 1];
    ptr->owned = true;
    ptr->type = internal;
  }
  /**
   *  Resizes this framebuffer and all added textures, that were created by this
   * framebuffer. Consider: Not very fast.
   */
  void resize(unsigned int width, unsigned int height) {
    this->width = width;
    this->height = height;
    std::vector<Attachement *> atch(attachements.size() + 1);
    for (Attachement *a : attachements) {
      if (a) {
        if (TextureAttachement *dta = dynamic_cast<TextureAttachement *>(a)) {
          if (dta->owned) {
            glBindTexture(GL_TEXTURE_2D, dta->id);
            glTexImage2D(GL_TEXTURE_2D, 0, dta->type, width, height, 0, GL_RGBA,
                         GL_FLOAT, nullptr);
            glBindTexture(GL_TEXTURE_2D, 0);
          }
        }
      }
    }
    if (depth) {
      if (TextureAttachement *dta = dynamic_cast<TextureAttachement *>(depth)) {
        if (dta->owned) {
          glBindTexture(GL_TEXTURE_2D, dta->id);
          glTexImage2D(GL_TEXTURE_2D, 0, dta->type, width, height, 0,
                       GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
          glBindTexture(GL_TEXTURE_2D, 0);
        }
      }
    }
  }
  /**
   *  Blits the contents of this Framebuffer to the specified Framebuffer
   */
  void blit(GLuint fbo, unsigned int other_width, unsigned int other_height) {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, this->id);
    glBlitFramebuffer(0, 0, width, height, 0, 0, other_width, other_height,
                      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                          GL_STENCIL_BUFFER_BIT,
                      GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
  /**
   *  Returns -1 if no depth attachement has been added or if the depth
   * attachement is a render buffer, else the OpenGL Texture handle.
   */
  GLint getDepthTexture() {
    return depth && depth->isTexture() ? depth->id : -1;
  }
  /**
   *  Returns -1 if no color attachement has been added to the n-th slot or if
   * the color attachement is a render buffer, else the OpenGL Texture handle.
   */
  GLint getColorTexture(int n = 0) {
    return n < attachements.size() && attachements[n]->isTexture()
               ? attachements[n]->id
               : -1;
  }
  /**
   *  Returns the current size of this framebuffer
   */
  glm::ivec2 getSize() { return {width, height}; }
};
#endif
