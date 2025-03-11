//#####################################################################
// OpenGL Textures
// Copyright (c) (2018-), Bo Zhu
//#####################################################################
#ifndef __OpenGLTexture_h__
#define __OpenGLTexture_h__
#include <string>
#include <memory>
#include "glad/glad.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Common.h"
#include "OpenGLShaderProgram.h"

class OpenGLTexture {
public:
	OpenGLTexture(GLuint texture) : texture(texture), target(GL_TEXTURE_2D) {}
	OpenGLTexture(GLuint texture, GLenum target) : texture(texture), target(target) {}
	~OpenGLTexture();

	void Bind(int textureSlot);
private:
	GLuint texture;
	GLenum target;
};


class OpenGLTextureLibrary
{
public:
	static OpenGLTextureLibrary* Instance();
	static std::shared_ptr<OpenGLTexture> Get_Texture(const std::string& name);
	std::shared_ptr<OpenGLTexture> Get(const std::string& name);
	void Add_Texture_From_File(std::string filename, std::string name);
	void Add_CubeMap_From_Files(const std::vector<std::string>& filenames, std::string name);
protected:
	Hashtable<std::string, std::shared_ptr<OpenGLTexture> > texture_hashtable;
};
#endif
