//#####################################################################
// OpenGL Textures
// Copyright (c) (2018-), Bo Zhu
//#####################################################################
#include <iostream>
#include "OpenGLWindow.h"
#include "OpenGLTexture.h"
#include <StbImage.h>

void OpenGLTexture::Bind(int textureSlot) {
	glActiveTexture(GL_TEXTURE0 + textureSlot); // activate the texture unit first before binding texture
	// glBindTexture(GL_TEXTURE_2D, texture); 
	glBindTexture(target, texture);
}

OpenGLTexture::~OpenGLTexture() {
	glDeleteTextures(0, &texture);
}

OpenGLTextureLibrary* OpenGLTextureLibrary::Instance() { static OpenGLTextureLibrary instance; return &instance; }
std::shared_ptr<OpenGLTexture> OpenGLTextureLibrary::Get(const std::string& name) {
	auto search=texture_hashtable.find(name);
	if (search != texture_hashtable.end())return search->second;
	else return std::shared_ptr<OpenGLTexture>(nullptr);
}

std::shared_ptr<OpenGLTexture> OpenGLTextureLibrary::Get_Texture(const std::string& name) {
	return OpenGLTextureLibrary::Instance()->Get(name);
}


void OpenGLTextureLibrary::Add_Texture_From_File(std::string filename, std::string name) {
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_CLAMP_TO_EDGE
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // GL_CLAMP_TO_EDGE
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load and generate the texture
	int width, height, nrChannels;
	unsigned char *data=nullptr;
	Stb::Read_Image(filename, width, height, nrChannels, data);

	if (data)
	{
		if (nrChannels == 4) 
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		else 
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cerr << "[Texture_Library]: Failed to load texture " << filename << std::endl;
	}

	free(data);

	texture_hashtable[name]=std::shared_ptr<OpenGLTexture>(new OpenGLTexture(texture));
}

void OpenGLTextureLibrary::Add_CubeMap_From_Files(const std::vector<std::string>& filenames, std::string name) {
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	// load and generate the texture
	int width, height, nrChannels;
	unsigned char *data=nullptr;
	for (int i=0; i<filenames.size(); i++)
	{
		Stb::Read_Image(filenames[i], width, height, nrChannels, data);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			std::cerr << "[Texture_Library]: Failed to load texture " << filenames[i] << std::endl;
		}
		free(data);
	}

	texture_hashtable[name]=std::shared_ptr<OpenGLTexture>(new OpenGLTexture(texture, GL_TEXTURE_CUBE_MAP));
}