#pragma once
#include <glm/glm.hpp>
#ifndef SHADER_HPP
#define SHADER_HPP
#include <GL/glew.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>
#include <vector>
#ifndef WIN32
#include <unistd.h>
#else
#define stat _stat
#endif
static long lastMod(std::string file) {
  struct stat result;
  if (!stat(file.c_str(), &result))
    return result.st_mtime;
  return -1;
}
static std::string loadFile(std::string &path, std::string secondLine = "") {
  using namespace std;
  ifstream file(path);
  string foo;
  bool first = true;
  for (string line; getline(file, line);) {
    bool included_smt = false;
    if (line[0] == '#') {
      size_t i = 1;
      const string includes = "include";
      bool is_include = true;
      for (; i < includes.size() + 1; i++)
        if (line.size() <= i || line[i] != includes[i - 1]) {
          is_include = false;
          break;
        }
      if (is_include && line.size() > i + 2 && line[i++] == ' ' &&
          line[i++] == '"') {
        string infn = "";
        for (; line.size() > i && line[i] != '"'; i++) {
          infn += line[i];
        }
        foo.append(loadFile(infn));
        foo.append("\n");
        included_smt = true;
      }
    }
    if (!included_smt) {
      foo.append(line);
      foo.append("\n");
    }
    if (first) {
      foo.append(secondLine);
      first = false;
    }
  }
  file.close();
  return foo;
}
static GLuint createShader(std::string file, GLuint type,
                           std::string preproc = "") {
  GLuint id = glCreateShader(type);
  std::string srcs = loadFile(file, preproc);
  const char *src = srcs.c_str();
  glShaderSource(id, 1, &src, nullptr);
  glCompileShader(id);
  int success;
  char infoLog[512];
  glGetShaderiv(id, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(id, 512, nullptr, infoLog);
    std::cerr << "Cloud not compile shader, log: " << std::string(infoLog)
              << std::endl;
  }
  return id;
}
class ShaderProgram {

protected:
  std::vector<std::string> attribs;
  std::unordered_map<std::string, GLint> uniformCache;
  uint activeTextures = 0;
  void _construct(
      std::vector<std::pair<std::string, GLuint>> shader,
      std::vector<std::string> predefattribs = std::vector<std::string>(),
      std::vector<std::string> defines = std::vector<std::string>()) {
    std::string preproc = "";
    for (std::string def : defines)
      preproc += "#define " + def + '\n';
    id = glCreateProgram();
    std::vector<GLuint> todel(shader.size());
    int i = 0;
    for (auto &shd : shader) {
      GLuint sid = createShader(shd.first, shd.second, preproc);
      glAttachShader(id, sid);
      todel[i++] = sid;
    }
    glLinkProgram(id);
    for (GLuint del : todel)
      glDeleteShader(del);
    int success;
    char infoLog[512];
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(id, 512, nullptr, infoLog);
      std::cerr << "Shader compilation failed, log: " << std::string(infoLog)
                << std::endl;
    }
    if (!predefattribs.empty())
      addAttributes(predefattribs);
  }

  ShaderProgram() {}

public:
  unsigned int id;
  /**
   *  Constructs a shader program consisting of possibly several shader stages
   * and optional predefined attributes
   *  @param shader vector entries each describe one shader stage as a pair of
   * the path to the shader source and the opengl shader type
   *  @param predefattribs if attributes should be directly bound to an index,
   * one can pass the list of atribute names in the correct order through this
   * parameter
   */
  ShaderProgram(
      std::vector<std::pair<std::string, GLuint>> shader,
      std::vector<std::string> predefattribs = std::vector<std::string>(),
      std::vector<std::string> defines = std::vector<std::string>()) {
    _construct(shader, predefattribs, defines);
  }
  ShaderProgram(const ShaderProgram &) = delete;
  ShaderProgram &operator=(const ShaderProgram &) = delete;
  ShaderProgram(ShaderProgram &&foo) = delete;
  ShaderProgram &operator=(ShaderProgram &&foo) = delete;
  /**
   *  Associates the attribute name with a index which equals
   *  the number of attributes already present in this program
   */
  void addAttribute(std::string attr) {
    int i = attribs.size();
    attribs.push_back(attr);
    glBindAttribLocation(id, i, attribs[i].c_str());
  }
  /**
   *  Associates the attribute names with the index which equals
   *  the number of attributes already present in this program
   */
  void addAttributes(std::vector<std::string> attr) {
    attribs.reserve(attribs.size() + attr.size());
    for (auto &foo : attr)
      addAttribute(foo);
  }
  /**
   *  Does NOT happen in the destructor,
   *  so you can copy and move programs
   */
  void cleanUp() { glDeleteProgram(id); }
  /**
   *  Tells OpenGL to draw the following draw calls
   *  with this program
   */
  void start() {
    glUseProgram(id);
    activeTextures = 0;
  }
  /**
   *  Not necessary
   */
  virtual void stop() const { glUseProgram(0); }
  /**
   *  Constructs a standard shader program consisting of vertex shader and
   * fragment shader.
   */
  ShaderProgram(
      std::string vertex, std::string fragment,
      std::vector<std::string> predefattribs = std::vector<std::string>(),
      std::vector<std::string> defines = std::vector<std::string>()) {
    _construct({{vertex, GL_VERTEX_SHADER}, {fragment, GL_FRAGMENT_SHADER}},
               predefattribs, defines);
  }
  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, float value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniform1f(i, value);
  }
  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, glm::vec2 value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniform2f(i, value.x, value.y);
  }
  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, glm::vec3 value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniform3f(i, value.x, value.y, value.z);
  }
  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, glm::vec4 value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniform4f(i, value.x, value.y, value.z, value.w);
  }
  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, int value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniform1i(i, value);
  }
  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, glm::ivec2 value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniform2i(i, value.x, value.y);
  }
  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, glm::ivec3 value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniform3i(i, value.x, value.y, value.z);
  }
  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, glm::ivec4 value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniform4i(i, value.x, value.y, value.z, value.w);
  }

  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, glm::mat2 value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniformMatrix2fv(i, 1, false, &(value[0][0]));
  }
  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, glm::mat3 value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniformMatrix3fv(i, 1, false, &(value[0][0]));
  }
  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, glm::mat4 value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniformMatrix4fv(i, 1, false, &(value[0][0]));
  }
  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, glm::mat2x3 value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniformMatrix2x3fv(i, 1, false, &(value[0][0]));
  }
  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, glm::mat2x4 value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniformMatrix2x4fv(i, 1, false, &(value[0][0]));
  }
  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, glm::mat3x2 value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniformMatrix3x2fv(i, 1, false, &(value[0][0]));
  }
  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, glm::mat3x4 value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniformMatrix3x4fv(i, 1, false, &(value[0][0]));
  }
  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, glm::mat4x2 value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniformMatrix4x2fv(i, 1, false, &(value[0][0]));
  }
  /**
   *  Loads a value to a uniform variable
   * @param id the identifier of that uniform variable
   */
  void load(std::string id, glm::mat4x3 value) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    glUniformMatrix4x3fv(i, 1, false, &(value[0][0]));
  }
  /**
   * Loads an image to a uniform variable. The image is automatically bound.
   * @param id the identifier of that uniform variable
   * @param tex the opengl id for the texture
   * @param unit the texture socket this texture should be loaded to. Will be
   * automatically assigned if -1.
   */
  void loadTexture(std::string id, GLuint tex, int unit = -1) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    if (unit < 0)
      unit = activeTextures;
    activeTextures++;
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(i, unit);
  }
  /**
   * Loads an image to a uniform variable. The image is automatically bound.
   * @param id the identifier of that uniform variable
   * @param tex the opengl id for the texture
   * @param unit the texture socket this texture should be loaded to. Will be
   * automatically assigned if -1.
   */
  void loadTexture3D(std::string id, GLuint tex, int unit = -1) {
    GLint i;
    if (uniformCache.find(id) == uniformCache.end()) {
      uniformCache.insert({id, glGetUniformLocation(this->id, id.c_str())});
      i = uniformCache[id];
      if (i == -1) {
        return;
      }
    } else
      i = uniformCache[id];
    if (unit < 0)
      unit = activeTextures;
    activeTextures++;
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_3D, tex);
    glUniform1i(i, unit);
  }
};
class ComputeShader : public ShaderProgram {
private:
  int drawingTextures = 0;

public:
  ComputeShader(std::string source) {
    _construct({{source, GL_COMPUTE_SHADER}});
  }
  ComputeShader(std::string source, std::vector<std::string> defs) {
    _construct({{source, GL_COMPUTE_SHADER}}, std::vector<std::string>(), defs);
  }
  void dispatch(GLuint workGroupX, GLuint workGroupY, GLuint workGroupZ = 1) {
    glDispatchCompute(workGroupX, workGroupY, workGroupZ);
    drawingTextures = 0;
  }
  void waitForBarriers() const { glMemoryBarrier(GL_ALL_BARRIER_BITS); }
  /**
   *  Waits for all memory barriers and unbinds this shader
   */
  virtual void stop() const {
    waitForBarriers();
    glUseProgram(0);
  }
  /**
   *  Binds the texture to the n-th image unit
   *  Can be called multiple times. The first bound texture per iteration will
   * be bound to 0, the second to 1, ....
   */
  void bindImage(GLuint tex, GLenum access = GL_READ_WRITE,
                 GLenum format = GL_RGBA32F, int unit = -1) {
    if (unit < 0)
      unit = drawingTextures;
    drawingTextures = unit + 1;
    glBindImageTexture(unit, tex, 0, GL_FALSE, 0, access, format);
  }
};
#endif
