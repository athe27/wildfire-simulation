/**
 * Molin Deng, https://molin7.vercel.app/
*/
#ifndef __OpenGLBgEffect_h__
#define __OpenGLBgEffect_h__
#include "glm.hpp"
#include <glad.h>
#include "Mesh.h"
#include "OpenGLObject.h"
#include "OpenGLBufferObjects.h"
#include "OpenGLTexture.h"

class OpenGLBgEffect : public OpenGLObject
{
public:
    typedef OpenGLObject Base;
    Box<2> box;
    real depth;

    GLfloat iTime = 0;
    GLint iFrame = 0;
    Vector2f iResolution = Vector2f(1280, 960);

    OpenGLBgEffect()
    {
        name = "bg_effect";
        box = Box<2>(Vector2::Ones() * (real)-1, Vector2::Ones());
        polygon_mode = PolygonMode::Fill;
        Set_Depth((real).9999);
    }

    void setResolution(float w, float h) { iResolution = Vector2f(w, h); }
    void setTime(GLfloat time) { iTime = time; }
    void setFrame(int frame) { iFrame = frame; }
    void Set_Depth(const real _depth) { depth = _depth; }
    void Initialize()
    {
        Base::Initialize();

        Update_Data_To_Render_Pre();

        Array<Vector3> vtx = {Vector3(box.min_corner[0], box.min_corner[1], depth),
                              Vector3(box.max_corner[0], box.min_corner[1], depth),
                              Vector3(box.max_corner[0], box.max_corner[1], depth),

                              Vector3(box.min_corner[0], box.min_corner[1], depth),
                              Vector3(box.max_corner[0], box.max_corner[1], depth),
                              Vector3(box.min_corner[0], box.max_corner[1], depth)};
        Array<Vector2> uv = {Vector2((real)0., (real)0.), Vector2((real)1., (real)0.), Vector2((real)1., (real)1.),
                             Vector2((real)0., (real)0.), Vector2((real)1., (real)1.), Vector2((real)0., (real)1.)};

        for (int i = 0; i < vtx.size(); i++)
        {
            OpenGL_Vertex4(vtx[i], opengl_vertices); ////position, 4 floats
            // OpenGL_Color4(color.rgba, opengl_vertices);
            OpenGL_Vertex4(uv[i], opengl_vertices);
        } ////color, 4 floats

        Set_OpenGL_Vertices();
        Set_OpenGL_Vertex_Attribute(0, 4, 8, 0); // position, layout(location = 0)
        Set_OpenGL_Vertex_Attribute(1, 4, 8, 4); // uv, layout(location = 1)

        Update_Data_To_Render_Post();
    }
    void Display() const
    {
        Update_Polygon_Mode();

        std::shared_ptr<OpenGLShaderProgram> shader = shader_programs[0];
        shader->Begin();
        glDepthMask(GL_FALSE);

        Enable_Alpha_Blend(); // enable alpha blending

        shader->Set_Uniform("iResolution", iResolution);
        shader->Set_Uniform("iTime", iTime);
        shader->Set_Uniform("iFrame", iFrame);

        for (int i = 1; i <= textures.size(); i++)
        {
            shader->Set_Uniform(textures[i - 1].binding_name, i);
            textures[i - 1].texture->Bind(i);
        }

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vtx_size / 8);
        glDepthMask(GL_TRUE);
        shader->End();
    }
};

#endif