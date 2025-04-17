//#####################################################################
// OpenGL Mesh
// Copyright (c) (2018-), Bo Zhu
//#####################################################################
#ifndef __OpenGLObjectMesh_h__
#define __OpenGLObjectMesh_h__
#include <string>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Mesh.h"
#include "OpenGLObject.h"
#include "OpenGLShaderProgram.h"
#include "OpenGLTexture.h"
#include "OpenGLBufferObjects.h"

const OpenGLColor default_mesh_color=OpenGLColor::Blue();

template<class T_MESH> class OpenGLMesh: public OpenGLObject
{public:typedef OpenGLObject Base;typedef T_MESH MESH_TYPE;using Base::Add_Shader_Program;
	T_MESH mesh;
	OpenGLMesh(){}

	virtual void Initialize()
	{
		Base::Initialize();
		Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("vcolor"));
		Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("vnormal_lt"));
		Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("sd_depth"));		/////SHADOW TODO: uncomment the depth shader
		//Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("sd_lt"));		
	}

	virtual void Update_Data_To_Render()
	{
		if(!Update_Data_To_Render_Pre())return;

		for(auto& p:mesh.Vertices()){
			OpenGL_Vertex4_And_Color4(p,color.rgba,opengl_vertices);}		////position, 4 floats; color, 4 floats
		for(auto& e:mesh.elements)OpenGL_Vertex(e,opengl_elements);

		Set_OpenGL_Vertices();
		Set_OpenGL_Vertex_Attribute(0,4,8,0);	////position
		Set_OpenGL_Vertex_Attribute(1,4,8,4);	////color
		Set_OpenGL_Elements();

		Update_Data_To_Render_Post();
	}

	virtual void Refresh(const int frame)
	{
		bool is_binary_file=(File::File_Extension_Name(name)!="txt");std::string file_name=output_dir+"/"+std::to_string(frame)+"/"+name;
		if(is_binary_file){
			if(File::File_Exists(file_name)){
				mesh.elements.clear();
				File::Read_Binary_From_File(file_name,mesh);
				Set_Data_Refreshed();
				if(verbose)std::cout<<"Read file "<<file_name<<std::endl;}}
	}
};

class OpenGLSegmentMesh : public OpenGLMesh<SegmentMesh<3> >
{public:typedef OpenGLMesh<SegmentMesh<3> > Base;
    OpenGLSegmentMesh(){color=default_mesh_color;name="segment_mesh";}

	virtual void Display() const
    {
		if(!visible||mesh.elements.empty())return;
		Update_Polygon_Mode();

		GLfloat old_line_width;glGetFloatv(GL_LINE_WIDTH,&old_line_width);
		{std::shared_ptr<OpenGLShaderProgram> shader=shader_programs[0];
		shader->Begin();
		// On MacOSX only 1.0f is a valid line width
#ifndef __APPLE__
		glLineWidth(line_width);
#endif
		//glDisable(GL_DEPTH_TEST);
		OpenGLUbos::Bind_Uniform_Block_To_Ubo(shader,"camera");
		glBindVertexArray(vao);
		glDrawElements(GL_LINES,ele_size,GL_UNSIGNED_INT,0);
		//glEnable(GL_DEPTH_TEST);
		glLineWidth(old_line_width);
		shader->End();}
    }
};

class OpenGLTriangleMesh : public OpenGLMesh<TriangleMesh<3> >
{public:typedef OpenGLMesh<TriangleMesh<3> > Base;
	std::shared_ptr<OpenGLFbos::OpenGLFbo> fbo;
	glm::mat4 shadow_pv;
	glm::mat4 model_matrix=glm::mat4(1.0f);

	glm::vec3 ka = glm::vec3(0.1f, 0.1f, 0.1f);		// object mateiral ambient coefficient
	glm::vec3 kd = glm::vec3(0.7f, 0.7f, 0.7f);		// object material diffuse coefficient
	glm::vec3 ks = glm::vec3(2.f, 2.f, 2.f);		// object material specular coefficient
	float shininess = 0.f;							// object material shininess coefficient

	Array<Vector4f> vtx_color;
	Array<Vector3> vtx_normal;

	GLfloat iTime=0;

	void setTime(GLfloat time) { iTime=time; }

	void Set_Model_Matrix(const Eigen::Matrix<float, 4, 4>& _model_matrix)
	{
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				model_matrix[j][i] = _model_matrix(i, j); // j,i = i,j
	}

	void Set_Ka(const Vector3f& color) { ka[0] = color[0]; ka[1] = color[1]; ka[2] = color[2]; }
	void Set_Kd(const Vector3f& color) { kd[0] = color[0]; kd[1] = color[1]; kd[2] = color[2]; }
	void Set_Ks(const Vector3f& color) { ks[0] = color[0]; ks[1] = color[1]; ks[2] = color[2]; }
	void Set_Shininess(const float s) { shininess = s; }

    OpenGLTriangleMesh(){color=default_mesh_color;name="triangle_mesh";shading_mode=ShadingMode::Lighting;}

	virtual void Set_Shading_Mode(const ShadingMode& _mode)
	{
		shading_mode=_mode;
		switch(shading_mode){
		case ShadingMode::Shadow:{use_preprocess=true;use_depth_fbo=true;}break;}
	}
	void Compute_Shadow_PV(glm::mat4& shadow_pv)	////change the viewport
	{
		using namespace OpenGLUbos;using namespace OpenGLFbos;
		Lights* lights=Get_Lights();if(lights==nullptr)return;
		Light* lt=lights->First_Shadow_Light();if(lt==nullptr)return;
		glm::vec3 light_pos=glm::vec3(lt->pos);

		fbo=Get_And_Bind_Fbo("depth",/*depth*/1);
		glViewport(0,0,fbo->width,fbo->height);

		glm::mat4 proj,view;
		float np=.001f,fp=10.f;
		float r=(float)fbo->height/(float)fbo->width;float w=5.f;float h=w*r;

		////TOFIX: fix bug for perspective
		if(lt->Get_Type()==0){proj=glm::ortho(-w,w,-h,h,np,fp);}	////directional light
		else{proj=glm::perspective(45.f*(float)3.1415927f/360.f,1.f/r,np,fp);fbo->Set_Near_And_Far_Plane(np,fp);}

		view=glm::lookAt(light_pos,glm::vec3(0.f),glm::vec3(1.f,0.f,0.f));
		shadow_pv=proj*view;
	}

	virtual void Preprocess()
	{
		using namespace OpenGLUbos;using namespace OpenGLFbos;
		if(shading_mode!=ShadingMode::Shadow)return;

		GLint viewport[4];glGetIntegerv(GL_VIEWPORT,viewport);
		Compute_Shadow_PV(shadow_pv);

		std::shared_ptr<OpenGLShaderProgram> shader=shader_programs[3];		/////SHADOW TODO: Set the index to be the shadow depth shader
		shader->Begin();
		Bind_Uniform_Block_To_Ubo(shader,"camera");
		shader->Set_Uniform_Matrix4f("shadow_pv",glm::value_ptr(shadow_pv));
		shader->Set_Uniform_Matrix4f("model",glm::value_ptr(model_matrix));
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES,ele_size,GL_UNSIGNED_INT,0);
		shader->End();
		Unbind_Fbo();

		glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
	}
	virtual void Update_Data_To_Render()
	{
		if(!Update_Data_To_Render_Pre())return;

		switch(shading_mode){
		case ShadingMode::None:
		{use_vtx_color=true;use_vtx_normal=false;use_vtx_tangent=false; use_vtx_tex =false;}break;
		case ShadingMode::Lighting:
		{use_vtx_color=false;use_vtx_normal=true; use_vtx_tangent=false; use_vtx_tex=false; }break;
		case ShadingMode::A2:
		case ShadingMode::Phong:
		{use_vtx_color=true;use_vtx_normal=true; use_vtx_tangent=false; use_vtx_tex=false;}break;
		case ShadingMode::Texture:
		case ShadingMode::TexAlpha:
		{use_vtx_color=true; use_vtx_normal=true; use_vtx_tangent=true; use_vtx_tex=true; }break;
		case ShadingMode::Shadow:
		{use_vtx_color=true;use_vtx_normal=true;use_vtx_tangent=true;use_vtx_tex=true;}break;
		}
		
		if(use_vtx_normal&&(mesh.Normals().size()<mesh.Vertices().size()||recomp_vtx_normal)){
			Update_Normals(mesh,mesh.Normals());}

		if ((use_vtx_tex || use_vtx_tangent) && (mesh.Uvs().size() < mesh.Vertices().size())) {
			Update_Uvs(mesh, mesh.Uvs());}

		if (use_vtx_tangent && (mesh.Tangents().size() < mesh.Vertices().size() || recomp_vtx_tangent)) {
			Update_Tangents(mesh);}

		bool doSkinning=mesh.Weights().size() != 0;

		GLuint stride_size=4+(use_vtx_color?4:0)+(use_vtx_normal?4:0)+(use_vtx_tex?4:0)+(use_vtx_tangent?4:0) + (doSkinning ? 8 : 0);
		int i=0;for(auto& p:mesh.Vertices()){
			OpenGL_Vertex4(p,opengl_vertices);	////position, 4 floats
			if(use_vtx_color){
				if(vtx_color.size()>0)OpenGL_Color4(&vtx_color[i][0],opengl_vertices);
				else OpenGL_Color4(color.rgba,opengl_vertices);
			}	////color, 4 floats
			if(use_vtx_normal){
				if(vtx_normal.size()>0)OpenGL_Vertex4(vtx_normal[i],opengl_vertices);
				else OpenGL_Vertex4(mesh.Normals()[i],opengl_vertices);
			}	////normal, 4 floats
			if (use_vtx_tex) {
				OpenGL_Vertex4(mesh.Uvs()[i], opengl_vertices);
			}	////uvs, 4 floats
			if (use_vtx_tangent) {
				OpenGL_Vertex4(mesh.Tangents()[i], opengl_vertices);}	////tangents, 4 floats
			if (doSkinning) {
				OpenGL_WeightsAndJoints(mesh.Weights()[i], mesh.Joints()[i], opengl_vertices); ////weights, 4 floats; joints, 4 ints
			}
			i++;}
		for(auto& e:mesh.elements)OpenGL_Vertex(e,opengl_elements);

		Set_OpenGL_Vertices();
		int idx=0;{Set_OpenGL_Vertex_Attribute(0,4,stride_size,0);idx++;}	////position
		if (use_vtx_color) { Set_OpenGL_Vertex_Attribute(idx,4,stride_size,idx*4);idx++;}	////color
		if (use_vtx_normal) { Set_OpenGL_Vertex_Attribute(idx, 4, stride_size, idx * 4); idx++; }	////normal
		if (use_vtx_tex) { Set_OpenGL_Vertex_Attribute(idx,4,stride_size,idx*4);idx++;}	////uvs
		if (use_vtx_tangent) { Set_OpenGL_Vertex_Attribute(idx, 4, stride_size, idx * 4); idx++; }	////tangent
		if (doSkinning) {
			Set_OpenGL_Vertex_Attribute(idx, 4, stride_size, idx * 4); idx++; // weights
			Set_OpenGL_Vertex_Attribute(idx, 4, stride_size, idx * 4); idx++; // joints
		}

		Set_OpenGL_Elements();
		Update_Data_To_Render_Post();
	}

	virtual void Display() const
    {
		using namespace OpenGLUbos;using namespace OpenGLFbos;
		if(!visible||mesh.elements.empty())return;
		Update_Polygon_Mode();

		switch(shading_mode){
		case ShadingMode::None:{
			std::shared_ptr<OpenGLShaderProgram> shader=shader_programs[0];
			shader->Begin();
			shader->Set_Uniform_Matrix4f("model",glm::value_ptr(model_matrix));
			Bind_Uniform_Block_To_Ubo(shader,"camera");
			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES,ele_size,GL_UNSIGNED_INT,0);
			shader->End();
		}break;
		case ShadingMode::A2:{
			std::shared_ptr<OpenGLShaderProgram> shader=shader_programs[0];
			shader->Begin();
			shader->Set_Uniform_Matrix4f("model",glm::value_ptr(model_matrix));
			shader->Set_Uniform("iTime", iTime);
			Bind_Uniform_Block_To_Ubo(shader,"camera");
			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES,ele_size,GL_UNSIGNED_INT,0);
			shader->End();		
		}break;
		case ShadingMode::Phong:{
			std::shared_ptr<OpenGLShaderProgram> shader=shader_programs[0];
			shader->Begin();
			shader->Set_Uniform_Matrix4f("model",glm::value_ptr(model_matrix));
			shader->Set_Uniform("iTime", iTime);
			shader->Set_Uniform("ka", ka);
			shader->Set_Uniform("kd", kd);
			shader->Set_Uniform("ks", ks);
			shader->Set_Uniform("shininess", shininess);
			Bind_Uniform_Block_To_Ubo(shader,"camera");
			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES,ele_size,GL_UNSIGNED_INT,0);
			shader->End();		
		}break;
		case ShadingMode::Texture:{
			std::shared_ptr<OpenGLShaderProgram> shader=shader_programs[0];
			shader->Begin();
			shader->Set_Uniform("iTime", iTime);

			for (int i=0; i < textures.size(); i++) {
				shader->Set_Uniform(textures[i].binding_name, i);
				textures[i].texture->Bind(i);
			}

			shader->Set_Uniform_Matrix4f("model",glm::value_ptr(model_matrix));
			shader->Set_Uniform("ka", ka);
			shader->Set_Uniform("kd", kd);
			shader->Set_Uniform("ks", ks);
			shader->Set_Uniform("shininess", shininess);

			Bind_Uniform_Block_To_Ubo(shader,"camera");
			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES,ele_size,GL_UNSIGNED_INT,0);
			shader->End();		
		}break;
		case ShadingMode::TexAlpha: {
			std::shared_ptr<OpenGLShaderProgram> shader=shader_programs[0];
			shader->Begin();
			Enable_Alpha_Blend(); // enable alpha blending

			for (int i=0; i < textures.size(); i++) {
				shader->Set_Uniform(textures[i].binding_name, i);
				textures[i].texture->Bind(i);
			}

			shader->Set_Uniform("iTime", iTime);
			shader->Set_Uniform_Matrix4f("model",glm::value_ptr(model_matrix));
			shader->Set_Uniform("ka", ka);
			shader->Set_Uniform("kd", kd);
			shader->Set_Uniform("ks", ks);
			shader->Set_Uniform("shininess", shininess);

			// bind cube map
			auto cube_map = OpenGLTextureLibrary::Get_Texture("cube_map");
			if (cube_map) {
				shader->Set_Uniform("skybox", (int)textures.size());
				cube_map->Bind((int)textures.size());
			}

			Bind_Uniform_Block_To_Ubo(shader,"camera");
			Bind_Uniform_Block_To_Ubo(shader,"lights");
			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES,ele_size,GL_UNSIGNED_INT,0);
			shader->End();	
		} break;
		case ShadingMode::Shadow:{
			std::shared_ptr<OpenGLShaderProgram> shader=shader_programs[0];		/////SHADOW TODO: Set the index to be the shadow depth shader
			shader->Begin();
			shader->Set_Uniform("iTime", iTime);
			for (int i=1; i <= textures.size(); i++) {
				shader->Set_Uniform(textures[i-1].binding_name, i);
				textures[i-1].texture->Bind(i);
			}
			
			shader->Set_Uniform_Matrix4f("model",glm::value_ptr(model_matrix));
			
			Bind_Uniform_Block_To_Ubo(shader,"camera");
			Bind_Uniform_Block_To_Ubo(shader,"lights");
			if(fbo!=nullptr)fbo->Bind_As_Texture(0);
			shader->Set_Uniform("shadow_map",0);

			shader->Set_Uniform_Matrix4f("shadow_pv",glm::value_ptr(shadow_pv));
			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES,ele_size,GL_UNSIGNED_INT,0);
			shader->End();
		}break;
		}
    }
};

class OpenGLScreenCover : public OpenGLMesh<TriangleMesh<3> >
{
public:typedef OpenGLMesh<TriangleMesh<3> > Base;
	std::shared_ptr<OpenGLFbos::OpenGLFbo> fbo;
	GLfloat iTime=0;
	GLint iFrame=0;
	Vector2f iResolution=Vector2f(1280, 960);
	GLuint FramebufferName=0;
	GLuint renderedTexture;
	bool use_tex = false;
	
	void setResolution(float w, float h) { iResolution=Vector2f(w, h); }

	void setTime(GLfloat time) { iTime=time; }

	void setFrame(int frame) { iFrame=frame; }

	void Add_Buffer() {
		glGenFramebuffers(1, &FramebufferName);
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

		glGenTextures(1, &renderedTexture);
		glBindTexture(GL_TEXTURE_2D, renderedTexture);
		// Give an empty image to OpenGL ( the last "0" )
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)iResolution[0], (GLsizei)iResolution[1], 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		// Poor filtering. Needed !
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);
		GLenum DrawBuffers[1]={ GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, DrawBuffers);
	}

	OpenGLScreenCover() {
		color=default_mesh_color; name="screen_cover"; shading_mode=ShadingMode::Lighting;
		mesh.Vertices().resize(3); mesh.Elements().resize(1); mesh.Elements()[0]=Vector3i(0, 1, 2);
	}

	virtual void Update_Data_To_Render()
	{
		if (!Update_Data_To_Render_Pre())return;

		GLuint stride_size=4;
		int i=0; for (auto& p : mesh.Vertices()) OpenGL_Vertex4(p, opengl_vertices);	////position, 4 floats
		for (auto& e : mesh.elements)OpenGL_Vertex(e, opengl_elements);

		Set_OpenGL_Vertices();
		int idx=0; {Set_OpenGL_Vertex_Attribute(0, 4, stride_size, 0); idx++; }	////position

		Set_OpenGL_Elements();
		Update_Data_To_Render_Post();
	}

	virtual void Display() const
	{
		Update_Polygon_Mode();
		// "Bind" the newly created texture : all future texture functions will modify this texture
		if (FramebufferName) {
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				return;
			glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
			glViewport(0, 0, (GLsizei)iResolution[0], (GLsizei)iResolution[1]);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, renderedTexture);
			std::shared_ptr<OpenGLShaderProgram> shader=shader_programs[4];
			shader->Begin();
			shader->Set_Uniform("iResolution", iResolution);
			shader->Set_Uniform("iTime", iTime);
			shader->Set_Uniform("iFrame", iFrame);
			if (use_tex) {
                // ! We must use i+1 since 0 slot is used by the renderedTexture
                for (int i = 0; i < textures.size(); i++) {
                    shader->Set_Uniform(textures[i].binding_name, i + 1);
                    textures[i].texture->Bind(i + 1);
                }
            }
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(1.f, 1.f);
			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES, ele_size, GL_UNSIGNED_INT, 0);
			glDisable(GL_POLYGON_OFFSET_FILL);
			shader->End();
		}

		std::shared_ptr<OpenGLShaderProgram> shader=shader_programs[3];
		shader->Begin();
		shader->Set_Uniform("iResolution", iResolution);
		shader->Set_Uniform("iTime", iTime);
		shader->Set_Uniform("iFrame", iFrame);
		if (FramebufferName)shader->Bind_Texture2D("bufferTexture", renderedTexture, 0);
		if (use_tex) {
			// ! also start from 1
			for (int i = 0; i < textures.size(); i++) {
				shader->Set_Uniform(textures[i].binding_name, i + 1);
				textures[i].texture->Bind(i + 1);
			}
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.f, 1.f);
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, ele_size, GL_UNSIGNED_INT, 0);
		glDisable(GL_POLYGON_OFFSET_FILL);
		shader->End();
	}
};

#endif
