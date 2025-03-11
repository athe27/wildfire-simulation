//#####################################################################
// OpenGL Interactive Object
// Copyright (c) (2018-), Bo Zhu
//#####################################################################
#ifndef __OpenGLMarkerObjects_h__
#define __OpenGLMarkerObjects_h__
#include "glm/glm.hpp"
#include "Mesh.h"
#include "OpenGLObject.h"

class OpenGLBackground : public OpenGLObject
{public:typedef OpenGLObject Base;
	Box<2> box;	
	std::string tex_name;
	std::string shader_name="gcolor_bk";
	real depth;
	bool use_fbo_tex;
	OpenGLColor mix_color = OpenGLColor(.01f, .01f, .2f, 1.f);

	OpenGLBackground();

	void Set_Box(const Vector2& min_corner,const Vector2& max_corner){box=Box<2>(min_corner,max_corner);}
	void Set_Texture(const std::string& _tex_name){use_vtx_tex=true;tex_name=_tex_name;}
	void Set_Depth(const real _depth){depth=_depth;}
	void Set_Fbo(){}
	void Set_Color(const OpenGLColor& _color, const OpenGLColor& _mix_color){color=_color; mix_color=_mix_color;}
	virtual void Initialize();
	virtual void Display() const;
};

class OpenGLAxes : public OpenGLObject
{public:typedef OpenGLObject Base;
	real axis_length=(real).5;
	bool use_2d_display=false;

	OpenGLAxes(){name="axes";polygon_mode=PolygonMode::Wireframe;}

	virtual void Initialize();
	virtual void Display() const;
};

class OpenGLPoint : public OpenGLObject
{public:typedef OpenGLObject Base;using Base::color;
	Vector3 pos=Vector3::Zero();
	GLfloat point_size=16.f;

	OpenGLPoint(){name="point";color=OpenGLColor::Red();polygon_mode=PolygonMode::Fill;}
	
	virtual void Initialize();
	virtual void Update_Data_To_Render();
	virtual void Display() const;
};

class OpenGLTriangle : public OpenGLObject
{public:typedef OpenGLObject Base;using Base::color;using Base::line_width;
	ArrayF<Vector3,3> vtx;

	OpenGLTriangle(){name="triangle";color=OpenGLColor::Red();polygon_mode=PolygonMode::Fill;}
	
	virtual void Initialize();
	virtual void Update_Data_To_Render();
	virtual void Display() const;
};

class OpenGLPolygon : public OpenGLObject
{public:typedef OpenGLObject Base;using Base::color;using Base::line_width;
	Array<Vector3> vtx;

	OpenGLPolygon(){name="polygon";color=OpenGLColor::Blue();polygon_mode=PolygonMode::Fill;}
	
	virtual void Initialize();
	virtual void Update_Data_To_Render();
	virtual void Display() const;
};

class OpenGLCircle : public OpenGLObject
{public: typedef OpenGLObject Base;
	Vector3 pos=Vector3::Zero();
	real radius=(real).1;
	glm::mat4 model=glm::mat4(1.f);

	int n=16;
	Array<Vector3> vtx;

	OpenGLCircle(){name="circle";color=OpenGLColor::Green();polygon_mode=PolygonMode::Fill;}

	virtual void Initialize();
	virtual void Update_Data_To_Render();
	virtual void Display() const;
	virtual void Update_Model_Matrix();
};

class OpenGLSolidCircle : public OpenGLObject
{public: typedef OpenGLObject Base;
	Vector3 pos=Vector3::Zero();
	real radius=(real).1;
	glm::mat4 model=glm::mat4(1.f);

	int n=16;
	Array<Vector3> vtx;

	OpenGLSolidCircle(){name="circle";color=OpenGLColor::Green();polygon_mode=PolygonMode::Fill;}

	virtual void Initialize();
	virtual void Update_Data_To_Render();
	virtual void Display() const;
	virtual void Update_Model_Matrix();
};

class OpenGLMarkerTriangleMesh : public OpenGLObject
{public:typedef OpenGLObject Base;using Base::color;
	TriangleMesh<3> mesh;
	glm::mat4 model=glm::mat4(1.f);

	OpenGLMarkerTriangleMesh(){name="interactive_triangle_mesh";color=OpenGLColor::Green();polygon_mode=PolygonMode::Fill;}
	
	virtual void Initialize();
	virtual void Update_Data_To_Render();
	virtual void Display() const;
	virtual void Update_Model_Matrix(){}

protected:
	virtual void Update_Mesh_Data_To_Render();
};

class OpenGLSphere : public OpenGLMarkerTriangleMesh
{public:typedef OpenGLMarkerTriangleMesh Base;
	using Base::color;using Base::mesh;using Base::model;
	Vector3 pos=Vector3::Zero();
	real radius=(real).1;

	OpenGLSphere(){name="sphere";color=OpenGLColor::Red();polygon_mode=PolygonMode::Fill;}
	
	virtual void Initialize();
	virtual void Update_Model_Matrix();
};

#endif
