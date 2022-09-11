#ifndef VAO_HPP
#define VAO_HPP
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>
template <typename T>
static unsigned int genVBO(unsigned int index, unsigned int stride, T *data,
                           unsigned int len) {
  static_assert(std::is_same<T, float>() || std::is_same<T, int>() ||
                    std::is_same<T, const float>() ||
                    std::is_same<T, const int>(),
                "Only float and int data is permitted in vbos!");
#ifdef DEBUG_BCE
  double start = glfwGetTime();
#endif
  unsigned int id;
  glGenBuffers(1, &id);
  glBindBuffer(GL_ARRAY_BUFFER, id);
  glBufferData(GL_ARRAY_BUFFER, len * sizeof(float), data, GL_STATIC_DRAW);
  glEnableVertexAttribArray(index);
  if (std::is_same<T, float>() || std::is_same<T, const float>())
    glVertexAttribPointer(index, stride, GL_FLOAT, GL_FALSE, 0, nullptr);
  else
    glVertexAttribIPointer(index, stride, GL_INT, 0, nullptr);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
#ifdef DEBUG_BCE
  global::gpu_time += glfwGetTime() - start;
#endif
  return id;
}
template <typename T>
static unsigned int genInstancedVBO(unsigned int index, unsigned int stride,
                                    T *data, unsigned int len,
                                    unsigned int divisor) {
  static_assert(std::is_same<T, float>() || std::is_same<T, int>() ||
                    std::is_same<T, const float>() ||
                    std::is_same<T, const int>(),
                "Only float and int data is permitted in vbos!");
  unsigned int id;
  glGenBuffers(1, &id);
  glBindBuffer(GL_ARRAY_BUFFER, id);
  glBufferData(GL_ARRAY_BUFFER, len * sizeof(T), data, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(index);
  if (std::is_same<T, float>() || std::is_same<T, const float>())
    glVertexAttribPointer(index, stride, GL_FLOAT, GL_FALSE, 0, nullptr);
  else
    glVertexAttribIPointer(index, stride, GL_INT, 0, nullptr);
  glVertexAttribDivisor(index, divisor);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  return id;
}
struct Vbo {
  const GLuint id;
  unsigned int index;
  bool instanced = false;
  GLuint dim;
  Vbo(const GLuint id, unsigned int index, GLuint dim, bool instanced = false)
      : id(id), index(index), dim(dim), instanced(instanced) {}
};
class Vao {
  GLuint id;
  std::vector<Vbo> vbos;
  std::optional<GLuint> indicesId;
  std::optional<long> instanceCount;
  long itemsCount = 0;

public:
  Vao() { glGenVertexArrays(1, &id); };
  void addIndexBuffer(const unsigned int *data, size_t count) {
    itemsCount = count;
    glBindVertexArray(id);
    GLuint indid;
    glGenBuffers(1, &indid);
    indicesId = std::optional<GLuint>(indid);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesId.value());
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }
  /**
   * Adds a Index Buffer to the Vao.
   * The Vao will be rendered according to if it has an index buffer or not.
   * @param indices index data to store
   */
  void addIndexBuffer(std::vector<unsigned int> indices) {
    addIndexBuffer(indices.data(), indices.size());
  }
  /**
   * Adds a vertex buffer to the Vao.
   * Only int and float datatypes are supported.
   * The index of this vbo is the count of vertex buffers already present.
   * @param dim   Dimension or stride of the vbo (e.g. 3 for a vec3)
   * @param data  Pointer to the data array
   * @param len   numbers of entries in the data array
   * @return the index of this vbo
   */
  template <typename T>
  unsigned int addVertexBuffer(unsigned int dim, const T *data,
                               unsigned int len) {
    glBindVertexArray(id);
    vbos.emplace_back(genVBO(vbos.size(), dim, data, len), vbos.size(), dim);
    if (!indicesId.has_value())
      itemsCount = len / dim;
    return vbos.size() - 1;
  }
  /**
   * Adds a vertex buffer to the Vao.
   * Only int and float datatypes are supported.
   * The index of this vbo is the count of vertex buffers already present.
   * @param dim   Dimension or stride of the vbo (e.g. 3 for a vec3)
   * @param data  the data array
   * @return the index of this vbo
   */
  template <typename T>
  unsigned int addVertexBuffer(unsigned int dim, std::vector<T> data) {
    return addVertexBuffer(dim, data.data(), data.size());
  }

  void setInstanceCount(int count) { instanceCount = count; }
  /**
   * Adds an instanced vertex buffer to the Vao.
   * The index of this vbo is the count of vertex buffers already present.
   * Note: if you use instanced vbos and vaos, dont forget to set the count of
   * objects with 'setInstanceCount(int)', or your data will not be rendered
   * correctly. Only int and float datatypes are supported.
   * @param dim   Dimension or stride of the vbo (e.g. 3 for a ivec3)
   * @param data  Pointer to the data array
   * @param len  numbers of entries in the data array
   * @param div   Attribute Divisor Count, default is 1
   * @return the index of this vbo
   */
  template <typename T>
  unsigned int addInstancedVertexBuffer(unsigned int dim, const T *data,
                                        unsigned int len, int div = 1) {
    glBindVertexArray(id);
    vbos.emplace_back(genInstancedVBO(vbos.size(), dim, data, len, div),
                      vbos.size(), dim, true);
    if (!instanceCount.has_value())
      instanceCount = 1;
    if (!indicesId.has_value())
      itemsCount = len / dim;
    return vbos.size() - 1;
  }
  /**
   * Adds an instanced vertex buffer to the Vao.
   * The index of this vbo is the count of vertex buffers already present.
   * Note: if you use instanced vbos and vaos, dont forget to set the count of
   * objects with 'setInstanceCount(int)', or your data will not be rendered
   * correctly.
   * @param dim   Dimension or stride of the vbo (e.g. 3 for a ivec3)
   * @param data  the data array
   * @param div   Attribute Divisor Count, default is 1
   * @return the index of this vbo
   */
  template <class T>
  unsigned int addInstancedVertexBuffer(unsigned int dim, std::vector<T> data,
                                        int div = 1) {
    return addInstancedVertexBuffer(dim, data.data(), data.size(), div);
  }
  /**
   * Updates the data of an instanced vbo
   * @param index the index of this vbo in the vao
   * @param data  the data that should be stored in the vbo
   * @param start byte offset
   * @param len   the count of entries of the data array
   */
  template <typename T>
  void updateVBO(int index, const T *data, size_t start, unsigned int len) {
    static_assert(std::is_same<T, float>() || std::is_same<T, int>(),
                  "Only float and int data is permitted in vbos!");
    glBindVertexArray(this->id);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[index].id);
    glBufferSubData(GL_ARRAY_BUFFER, start, sizeof(T) * len, &data[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }
  /**
   * Updates the data of an instanced vbo
   * @param index the index of this vbo i.e. the number at which this vbo has
   * been added to this vao
   * @param data  the data that should be stored in the vbo
   * @param len   the count of entries of the data array
   */
  template <typename T>
  void updateVBO(int index, const T *data, unsigned int len) {
    static_assert(std::is_same<T, float>() || std::is_same<T, int>(),
                  "Only float and int data is permitted in vbos!");
    glBindVertexArray(this->id);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[index].id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(T) * len, &(data[0]), GL_DYNAMIC_DRAW);
    if (!indicesId.has_value())
      itemsCount = len / vbos[index].dim;
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }
  /**
   * Updates the data of an instanced vbo
   * @param index the index of this vbo i.e. the number at which this vbo has
   * been added to this vao
   * @param data  the data that should be stored in the vbo
   */
  template <typename T> void updateVBO(int index, std::vector<T> data) {
    updateVBO(index, data.data(), data.size());
  }
  /**
   * Updates the data of an instanced vbo
   * @param index the index of this vbo in the vao
   * @param data  the data that should be stored in the vbo
   * @param start byte offset
   * @param len   the count of entries of the data array
   */
  template <typename T>
  void updateVBO(int index, std::vector<T> data, size_t start) {
    updateVBO(index, data.data(), start, data.size());
  }
  /**
   * Replaces Destructor, so you can safely copy a vao.
   * Cleans up all OpenGL related data.
   */
  void cleanUp() {
    for (Vbo v : vbos)
      glDeleteBuffers(1, &v.id);
    if (indicesId.has_value())
      glDeleteBuffers(1, &indicesId.value());
    glDeleteVertexArrays(1, &id);
  }
  void bind() { glBindVertexArray(id); }
  /**
   * Not actually necessary
   */
  void unbind() { glBindVertexArray(0); }
  /**
   * Draws the vao data corresponding to the present vbos
   */
  void draw(GLenum mode = GL_TRIANGLES) {
    if (instanceCount.has_value()) {
      if (indicesId.has_value()) {
        glDrawElementsInstanced(mode, itemsCount, GL_UNSIGNED_INT, nullptr,
                                instanceCount.value());
      } else
        glDrawArraysInstanced(mode, 0, itemsCount, instanceCount.value());
    } else {
      if (indicesId.has_value()) {
        glDrawElements(mode, itemsCount, GL_UNSIGNED_INT, nullptr);
      } else
        glDrawArrays(mode, 0, itemsCount);
    }
  }
};
#endif
