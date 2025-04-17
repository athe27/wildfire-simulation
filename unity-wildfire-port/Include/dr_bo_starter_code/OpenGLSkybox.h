
/**
 * Molin Deng, https://molin7.vercel.app/
*/
#ifndef __OpenGLSkybox_h__
#define __OpenGLSkybox_h__

#include "glm.hpp"
#include "Mesh.h"
#include "OpenGLObject.h"
#include "OpenGLShaderProgram.h"
#include "OpenGLTexture.h"
#include "OpenGLBufferObjects.h"

class OpenGLSkybox : public OpenGLObject
{
public:
    typedef OpenGLObject Base;

    bool use_self_defined_shader = false;
    GLfloat iTime = 0;

    void setTime(GLfloat time) { iTime = time; }


    OpenGLSkybox()
    {
        polygon_mode = PolygonMode::Fill;
    }

    void Initialize()
    {
        Base::Initialize();
        if (!use_self_defined_shader)
        {
            Add_Shader_Program(OpenGLShaderLibrary::Instance()->Get_Shader("skybox_default"));
        }
        Update_Data_To_Render_Pre();
        float vtx[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
        };
        for (auto i = 0; i < 36; i++)
        {
            OpenGL_Vertex4(Vector3(vtx[i * 3], vtx[i * 3 + 1], vtx[i * 3 + 2]), opengl_vertices);
        }
        Set_OpenGL_Vertices();
        Set_OpenGL_Vertex_Attribute(0, 4, 4, 0); ////position
        Update_Data_To_Render_Post();
    }

    void Add_Shader_Program(std::shared_ptr<OpenGLShaderProgram> shader_program) override
    {
        Base::Add_Shader_Program(shader_program);
        use_self_defined_shader = true;
    }

    void Display() const
    {
        Update_Polygon_Mode();

        std::shared_ptr<OpenGLShaderProgram> shader = shader_programs[0];
        shader->Begin();
        glDepthFunc(GL_LEQUAL);
        // glDepthMask(GL_FALSE);

        glBindVertexArray(vao);

        shader->Set_Uniform("iTime", iTime);
        
        auto cube_map = OpenGLTextureLibrary::Get_Texture("cube_map");
        if (cube_map)
        {
            shader->Set_Uniform("skybox", (int)textures.size());
            cube_map->Bind((int)textures.size());
        }
        OpenGLUbos::Bind_Uniform_Block_To_Ubo(shader, "camera");

        glDrawArrays(GL_TRIANGLES, 0, 36); // vtx_size / 4, vtx_size should 144
        // glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        shader->End();
    }
};

#endif
