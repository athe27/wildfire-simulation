//#####################################################################
// OpenGL Buffer Objects -- FBO and UBO
// Copyright (c) (2018-), Bo Zhu
//#####################################################################
#ifndef __OpenGLBufferObjects_h__
#define __OpenGLBufferObjects_h__
#include <string>
#include <memory>
#include "glad/glad.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Common.h"
#include "OpenGLShaderProgram.h"

namespace OpenGLFbos{
class OpenGLFbo
{public:
	GLuint buffer_index=0;
	std::string name="";
	GLsizei width=0;
	GLsizei height=0;
	GLuint tex_index=0;
	GLuint rbo_index=0;

	virtual void Initialize(const std::string& _name,GLuint _width=0,GLuint _height=0){}
	virtual void Resize(GLuint width,GLuint height){}
	virtual void Resize_To_Window(){}
	virtual void Write_To_File(const std::string& file_name){}
	virtual void Bind_As_Texture(GLuint idx=0){}
	virtual void Bind(){}
	virtual void Unbind(){}
	virtual void Clear(){}
	virtual void Set_Near_And_Far_Plane(float _near,float _far){}
};

class OpenGLFboInstance : public OpenGLFbo
{public:typedef OpenGLFbo Base;
	enum AttachmentType{Color=0,Depth,Position,Stencil} type;
	bool use_linearize_plane=false;
	float near_plane=.01f,far_plane=10.f;	////for depth linearization

	OpenGLFboInstance(AttachmentType _type=Color):Base(),type(_type){}
	virtual void Initialize(const std::string& _name,GLuint _width=0,GLuint _height=0);
	virtual void Resize(GLuint _width,GLuint _height);
	virtual void Clear();
	virtual void Resize_To_Window();
	virtual void Write_To_File(const std::string& file_name);
	virtual void Bind_As_Texture(GLuint idx=0);
	virtual void Set_Near_And_Far_Plane(float _near,float _far);
	virtual void Bind();
	virtual void Unbind();

protected:
	GLuint Generate_Attachment_Texture(const AttachmentType& att_type,GLuint width,GLuint height);
	void Bind_Texture_Color();
	void Bind_Texture_Depth();
	float Linearize_Depth(float depth,float near_plane,float far_plane);
};

class Fbo_Library
{public:
	static Fbo_Library* Instance();
	std::shared_ptr<OpenGLFbo> Get(const std::string& name,const int init_type=0);
protected:
	Hashtable<std::string,std::shared_ptr<OpenGLFbo> > fbo_hashtable;
	std::shared_ptr<OpenGLFbo> Lazy_Initialize_Fbo(const std::string& name,const int type);
};

std::shared_ptr<OpenGLFbo> Get_Fbo(const std::string& name,const int init_type=0);	////0-color,1-depth
std::shared_ptr<OpenGLFbo> Get_Depth_Fbo(const std::string& name);	////0-color,1-depth
OpenGLFboInstance* Get_Fbo_Instance(const std::string& name,const int init_type=0);
void Bind_Fbo(const std::string& name,const int init_type=0);
std::shared_ptr<OpenGLFbo> Get_And_Bind_Fbo(const std::string& name,const int init_type=0);
void Unbind_Fbo();
void Clear_Fbo(const std::string& name,const int init_type=0);
};

class OpenGLShaderProgram;

namespace OpenGLUbos{
#define To_String(S) #S

////Ubo class and uniform block declaration 
class Camera
{public:
	glm::mat4x4 projection;
	glm::mat4x4 view;
	glm::mat4x4 pvm;
	glm::mat4x4 ortho;
	glm::vec4 position;
};

const std::string camera=To_String(
layout (std140) uniform camera
{
	mat4 projection;
	mat4 view;
	mat4 pvm;
	mat4 ortho;
	vec4 position;
};
);

class Mat
{public:
	glm::vec4 amb;
	glm::vec4 dif;
	glm::vec4 spec;
	Mat():amb(1.f),dif(.5f),spec(.01f){}
};

class Light
{public:
	glm::ivec4 att;	////0-type, 1-has_shadow, type: 0-directional, 1-point, 2-spot
	glm::vec4 pos;
	glm::vec4 dir;
	glm::vec4 amb;
	glm::vec4 dif;
	glm::vec4 spec;
	glm::vec4 atten;	////0-const, 1-linear, 2-quad
	glm::vec4 r;	////0-inner,1-outer,2-(r[0]-r[1])

	Light(){Initialize();}

	void Set_Type(const int i){att[0]=i;}
	int Get_Type() const {return att[0];}
	void Set_Directional(){Set_Type(0);}
	void Set_Point(){Set_Type(1);}
	void Set_Spot(){Set_Type(2);}

	void Set_Shadow(bool use=true){att[1]=use?1:0;}
	bool Has_Shadow(){return att[1]!=0;}

	void Set_Cone(float inner_rad,float outer_rad){r[0]=cos(inner_rad);r[1]=cos(outer_rad);r[2]=r[0]-r[1];}

	void Initialize()
	{
		att=glm::ivec4(0,0,0,0);
		pos=glm::vec4(0.f,1.f,0.f,1.f);
		dir=glm::vec4(0.f,-1.f,0.f,1.f);
		amb=glm::vec4(.0f);
		dif=glm::vec4(1.f);
		spec=glm::vec4(1.f);
		atten=glm::vec4(1.f,.08f,.032f,0.f);
		r=glm::vec4(0.f);Set_Cone((float)(3.1415927f/6.f),(float)(.25*3.1415927f));
	}
};

class Lights
{public:
	glm::vec4 amb;
	glm::ivec4 lt_att;	////lt_att[0]: lt num
	Light lt[4];

	Lights():amb(.1f,.1f,.1f,1.f),lt_att(0){}
	int& Light_Num() {return lt_att[0];}
	const int& Light_Num() const {return lt_att[0];}
	int Last(){return Light_Num()-1;}
	Light* Get(const int i){if(i>Light_Num()-1)return nullptr;return &lt[i];}
	Light* First_Shadow_Light(){for(int i=0;i<Light_Num();i++)if(lt[i].Has_Shadow())return &lt[i];return nullptr;}
};

const std::string lights=To_String(
struct light
{
	ivec4 att;
	vec4 pos;
	vec4 dir;
	vec4 amb;
	vec4 dif;
	vec4 spec;
	vec4 atten;
	vec4 r;
};
layout (std140) uniform lights
{
	vec4 amb;
	ivec4 lt_att;	////lt_att[0]: lt num
	light lt[4];
};
);

//////////////////////////////////////////////////////////////////////////
////OpenGL UBO classes and library

class OpenGLUbo
{public:
	std::string name="";
	GLuint buffer_index=0;
	size_type buffer_size=0;	////bytes
	GLuint binding_point=0;
	GLint block_offset=0;
	size_type block_size=0;

	virtual void Initialize(const std::string& _uniform_block_name,GLuint _binding_point=0,GLuint _ubo=0,GLint _block_offset=0){}
	virtual void Set_Block_Attributes(){}
};

template<class T_UBO> class OpenGLUboInstance : public OpenGLUbo
{public:typedef OpenGLUbo Base;
	T_UBO object;
	OpenGLUboInstance():Base(){}

	virtual void Initialize(const std::string& _uniform_block_name,GLuint _binding_point=0,GLuint _ubo=0,GLint _block_offset=0);
	void Set_Block_Attributes();
	void Bind_Block();

protected:
	void Bind_Block(GLuint binding_point,GLint block_offset,size_type block_size);

	template<class T_ATT> GLint Set_Block_Attribute(const T_ATT& att,const GLint att_offset)	////work for glm types only, std140
	{
		size_type att_size=Size_Helper<T_ATT>();
		GLvoid* att_data=Data_Helper(att);
		glBindBuffer(GL_UNIFORM_BUFFER,buffer_index);
		glBufferSubData(GL_UNIFORM_BUFFER,att_offset,att_size,att_data);
		glBindBuffer(GL_UNIFORM_BUFFER,0);
		return att_offset+(GLint)att_size;
	}

	void Set_Block_Attributes_Helper(const Camera& obj,GLint block_offset);

	void Set_Block_Attributes_Helper(const Lights& obj,GLint block_offset);

	template<class T_VAL> GLvoid* Data_Helper(T_VAL& val){return (GLvoid*)(&val);}
	GLvoid* Data_Helper(glm::mat4& val){return (GLvoid*)(glm::value_ptr(val));}
	template<class T_ATT> size_type Size_Helper(){size_type size=sizeof(T_ATT);size=size<4?4:size;return size;}	////for std140
};

class Ubo_Library
{public:
	static Ubo_Library* Instance(){static Ubo_Library instance;return &instance;}
	std::shared_ptr<OpenGLUbo> Get(const std::string& name);
	GLuint Get_Binding_Point(const std::string& name);

protected:
	Hashtable<std::string,std::shared_ptr<OpenGLUbo> > ubo_hashtable;
	Hashtable<std::string,std::string> shader_header_hashtable;

	Ubo_Library();
	void Initialize_Ubos();
};

//////////////////////////////////////////////////////////////////////////
////UBO functions

////Global access
void Initialize_Ubos();
void Bind_Shader_Ubo_Headers(Hashtable<std::string,std::string>& shader_header_hashtable);
std::shared_ptr<OpenGLUbo> Get_Ubo(const std::string& name);
GLuint Get_Ubo_Binding_Point(const std::string& name);
bool Bind_Uniform_Block_To_Ubo(std::shared_ptr<OpenGLShaderProgram>& shader,const std::string& ubo_name);	////assuming uniform block name=ubo name

////Camera
OpenGLUboInstance<Camera>* Get_Camera_Ubo();
Camera* Get_Camera();
Vector3 Get_Camera_Pos();

////Lighting
OpenGLUboInstance<Lights>* Get_Lights_Ubo();
Lights* Get_Lights();
void Update_Lights_Ubo();
Light* Get_Light(const int i);
void Clear_Lights();
Light* Add_Directional_Light(const glm::vec3& dir);
Light* Add_Point_Light(const glm::vec3& pos);
Light* Add_Spot_Light(const glm::vec3& pos,glm::vec3& dir);
void Set_Ambient(const glm::vec4& amb);
};
#endif
