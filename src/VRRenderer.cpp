#include "VRRenderer.h"

#include "DebugCallback.h"

#include <QtGui/QMatrix4x4>

#pragma comment(lib, "user32.lib")

#if defined(_WIN32)
#include <dxgi.h> // for GetDefaultAdapterLuid
#pragma comment(lib, "dxgi.lib")
#endif

#ifndef VALIDATE
#define VALIDATE(x, msg) if (!(x)) { MessageBoxA(nullptr, (msg), "QuickVR", MB_ICONERROR | MB_OK); exit(-1); }
#endif

struct OculusTextureBuffer : protected QOpenGLExtraFunctions
{
    ovrSession          Session;
    ovrTextureSwapChain ColorTextureChain;
    ovrTextureSwapChain DepthTextureChain;
    GLuint              fboId;
    OVR::Sizei          texSize;

    OculusTextureBuffer(ovrSession session, OVR::Sizei size, int sampleCount) :
        Session(session),
        ColorTextureChain(nullptr),
        DepthTextureChain(nullptr),
        fboId(0),
        texSize(0, 0)
    {
        assert(sampleCount <= 1); // The code doesn't currently handle MSAA textures.

        texSize = size;

        // This texture isn't necessarily going to be a rendertarget, but it usually is.
        assert(session); // No HMD? A little odd.

        initializeOpenGLFunctions();

        ovrTextureSwapChainDesc desc = {};
        desc.Type = ovrTexture_2D;
        desc.ArraySize = 1;
        desc.Width = size.w;
        desc.Height = size.h;
        desc.MipLevels = 1;
        desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
        desc.SampleCount = sampleCount;
        desc.StaticImage = ovrFalse;

        {
            ovrResult result = ovr_CreateTextureSwapChainGL(Session, &desc, &ColorTextureChain);

            int length = 0;
            ovr_GetTextureSwapChainLength(session, ColorTextureChain, &length);

            if(OVR_SUCCESS(result))
            {
                for (int i = 0; i < length; ++i)
                {
                    GLuint chainTexId;
                    ovr_GetTextureSwapChainBufferGL(Session, ColorTextureChain, i, &chainTexId);
                    glBindTexture(GL_TEXTURE_2D, chainTexId);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                }
            }
        }

        desc.Format = OVR_FORMAT_D32_FLOAT;

        {
            ovrResult result = ovr_CreateTextureSwapChainGL(Session, &desc, &DepthTextureChain);

            int length = 0;
            ovr_GetTextureSwapChainLength(session, DepthTextureChain, &length);

            if (OVR_SUCCESS(result))
            {
                for (int i = 0; i < length; ++i)
                {
                    GLuint chainTexId;
                    ovr_GetTextureSwapChainBufferGL(Session, DepthTextureChain, i, &chainTexId);
                    glBindTexture(GL_TEXTURE_2D, chainTexId);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                }
            }
        }

        glGenFramebuffers(1, &fboId);
    }

    ~OculusTextureBuffer()
    {
        if (ColorTextureChain)
        {
            ovr_DestroyTextureSwapChain(Session, ColorTextureChain);
            ColorTextureChain = nullptr;
        }
        if (DepthTextureChain)
        {
            ovr_DestroyTextureSwapChain(Session, DepthTextureChain);
            DepthTextureChain = nullptr;
        }
        if (fboId)
        {
            glDeleteFramebuffers(1, &fboId);
            fboId = 0;
        }
    }

    OVR::Sizei GetSize() const
    {
        return texSize;
    }

    void SetAndClearRenderSurface()
    {
        GLuint curColorTexId;
        GLuint curDepthTexId;
        {
            int curIndex;
            ovr_GetTextureSwapChainCurrentIndex(Session, ColorTextureChain, &curIndex);
            ovr_GetTextureSwapChainBufferGL(Session, ColorTextureChain, curIndex, &curColorTexId);
        }
        {
            int curIndex;
            ovr_GetTextureSwapChainCurrentIndex(Session, DepthTextureChain, &curIndex);
            ovr_GetTextureSwapChainBufferGL(Session, DepthTextureChain, curIndex, &curDepthTexId);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curColorTexId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, curDepthTexId, 0);

        glViewport(0, 0, texSize.w, texSize.h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_FRAMEBUFFER_SRGB);
    }

    void UnsetRenderSurface()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    }

    void Commit()
    {
        ovr_CommitTextureSwapChain(Session, ColorTextureChain);
        ovr_CommitTextureSwapChain(Session, DepthTextureChain);
    }
};

static ovrGraphicsLuid GetDefaultAdapterLuid()
{
    ovrGraphicsLuid luid = ovrGraphicsLuid();

#if defined(_WIN32)
    IDXGIFactory* factory = nullptr;

    if (SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&factory))))
    {
        IDXGIAdapter* adapter = nullptr;

        if (SUCCEEDED(factory->EnumAdapters(0, &adapter)))
        {
            DXGI_ADAPTER_DESC desc;

            adapter->GetDesc(&desc);
            memcpy(&luid, &desc.AdapterLuid, sizeof(luid));
            adapter->Release();
        }

        factory->Release();
    }
#endif

    return luid;
}


static int Compare(const ovrGraphicsLuid& lhs, const ovrGraphicsLuid& rhs)
{
    return memcmp(&lhs, &rhs, sizeof(ovrGraphicsLuid));
}

struct DepthBuffer : protected QOpenGLExtraFunctions
{
    GLuint        texId;

    DepthBuffer(OVR::Sizei size)
    {
        initializeOpenGLFunctions();
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        GLenum internalFormat = GL_DEPTH_COMPONENT24;
        GLenum type = GL_UNSIGNED_INT;
        if (GL_ARB_depth_buffer_float)
        {
            internalFormat = GL_DEPTH_COMPONENT32F;
            type = GL_FLOAT;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.w, size.h, 0, GL_DEPTH_COMPONENT, type, NULL);
    }
    ~DepthBuffer()
    {
        if (texId)
        {
            glDeleteTextures(1, &texId);
            texId = 0;
        }
    }
};

struct TextureBuffer : protected QOpenGLExtraFunctions
{
    GLuint              texId;
    GLuint              fboId;
    OVR::Sizei          texSize;

    TextureBuffer(bool rendertarget, OVR::Sizei size, int mipLevels, unsigned char * data) :
        texId(0),
        fboId(0),
        texSize(0, 0)
    {
        initializeOpenGLFunctions();
        texSize = size;

        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);

        if (rendertarget)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, texSize.w, texSize.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        if (mipLevels > 1)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        if(rendertarget)
        {
            glGenFramebuffers(1, &fboId);
        }
    }

    ~TextureBuffer()
    {
        if (texId)
        {
            glDeleteTextures(1, &texId);
            texId = 0;
        }
        if (fboId)
        {
            glDeleteFramebuffers(1, &fboId);
            fboId = 0;
        }
    }

    OVR::Sizei GetSize() const
    {
        return texSize;
    }

    void SetAndClearRenderSurface(DepthBuffer* dbuffer)
    {
        VALIDATE(fboId, "Texture wasn't created as a render target");

        GLuint curTexId = texId;

        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dbuffer->texId, 0);

        glViewport(0, 0, texSize.w, texSize.h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_FRAMEBUFFER_SRGB);
    }

    void UnsetRenderSurface()
    {
        VALIDATE(fboId, "Texture wasn't created as a render target");

        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    }
};

struct ShaderFill : protected QOpenGLExtraFunctions
{
    GLuint            program;
    TextureBuffer   * texture;

    ShaderFill(GLuint vertexShader, GLuint pixelShader, TextureBuffer* _texture)
    {
        initializeOpenGLFunctions();

        texture = _texture;

        program = glCreateProgram();

        glAttachShader(program, vertexShader);
        glAttachShader(program, pixelShader);

        glLinkProgram(program);

        glDetachShader(program, vertexShader);
        glDetachShader(program, pixelShader);

        GLint r;
        glGetProgramiv(program, GL_LINK_STATUS, &r);
        if (!r)
        {
            GLchar msg[1024];
            glGetProgramInfoLog(program, sizeof(msg), 0, msg);
            qDebug("Linking shaders failed: %s\n", msg);
        }
    }

    ~ShaderFill()
    {
        if (program)
        {
            glDeleteProgram(program);
            program = 0;
        }
        if (texture)
        {
            delete texture;
            texture = nullptr;
        }
    }
};

//----------------------------------------------------------------
struct VertexBuffer : protected QOpenGLExtraFunctions
{
    GLuint    buffer;

    VertexBuffer(void* vertices, size_t size)
    {
        initializeOpenGLFunctions();
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    }
    ~VertexBuffer()
    {
        if (buffer)
        {
            glDeleteBuffers(1, &buffer);
            buffer = 0;
        }
    }
};

//----------------------------------------------------------------
struct IndexBuffer: protected QOpenGLExtraFunctions
{
    GLuint    buffer;

    IndexBuffer(void* indices, size_t size)
    {
        initializeOpenGLFunctions();
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
    }
    ~IndexBuffer()
    {
        if (buffer)
        {
            glDeleteBuffers(1, &buffer);
            buffer = 0;
        }
    }
};

struct Model: protected QOpenGLExtraFunctions
{
    struct Vertex
    {
        OVR::Vector3f  Pos;
        DWORD     C;
        float     U, V;
    };

    OVR::Vector3f        Pos;
    OVR::Quatf           Rot;
    OVR::Matrix4f        Mat;
    int             numVertices, numIndices;
    Vertex          Vertices[2000]; // Note fixed maximum
    GLushort        Indices[2000];
    ShaderFill    * Fill;
    VertexBuffer  * vertexBuffer;
    IndexBuffer   * indexBuffer;

    Model(OVR::Vector3f pos, ShaderFill * fill) :
        numVertices(0),
        numIndices(0),
        Pos(pos),
        Rot(),
        Mat(),
        Fill(fill),
        vertexBuffer(nullptr),
        indexBuffer(nullptr)
    {
        initializeOpenGLFunctions();
    }

    ~Model()
    {
        FreeBuffers();
    }

    OVR::Matrix4f& GetMatrix()
    {
        Mat = OVR::Matrix4f(Rot);
        Mat = OVR::Matrix4f::Translation(Pos) * Mat;
        return Mat;
    }

    void AddVertex(const Vertex& v) { Vertices[numVertices++] = v; }
    void AddIndex(GLushort a) { Indices[numIndices++] = a; }

    void AllocateBuffers()
    {
        vertexBuffer = new VertexBuffer(&Vertices[0], numVertices * sizeof(Vertices[0]));
        indexBuffer = new IndexBuffer(&Indices[0], numIndices * sizeof(Indices[0]));
    }

    void FreeBuffers()
    {
        delete vertexBuffer; vertexBuffer = nullptr;
        delete indexBuffer; indexBuffer = nullptr;
    }

    void AddSolidColorBox(float x1, float y1, float z1, float x2, float y2, float z2, DWORD c)
    {
        OVR::Vector3f Vert[][2] =
            {
                OVR::Vector3f(x1, y2, z1), OVR::Vector3f(z1, x1), OVR::Vector3f(x2, y2, z1), OVR::Vector3f(z1, x2),
                OVR::Vector3f(x2, y2, z2), OVR::Vector3f(z2, x2), OVR::Vector3f(x1, y2, z2), OVR::Vector3f(z2, x1),
                OVR::Vector3f(x1, y1, z1), OVR::Vector3f(z1, x1), OVR::Vector3f(x2, y1, z1), OVR::Vector3f(z1, x2),
                OVR::Vector3f(x2, y1, z2), OVR::Vector3f(z2, x2), OVR::Vector3f(x1, y1, z2), OVR::Vector3f(z2, x1),
                OVR::Vector3f(x1, y1, z2), OVR::Vector3f(z2, y1), OVR::Vector3f(x1, y1, z1), OVR::Vector3f(z1, y1),
                OVR::Vector3f(x1, y2, z1), OVR::Vector3f(z1, y2), OVR::Vector3f(x1, y2, z2), OVR::Vector3f(z2, y2),
                OVR::Vector3f(x2, y1, z2), OVR::Vector3f(z2, y1), OVR::Vector3f(x2, y1, z1), OVR::Vector3f(z1, y1),
                OVR::Vector3f(x2, y2, z1), OVR::Vector3f(z1, y2), OVR::Vector3f(x2, y2, z2), OVR::Vector3f(z2, y2),
                OVR::Vector3f(x1, y1, z1), OVR::Vector3f(x1, y1), OVR::Vector3f(x2, y1, z1), OVR::Vector3f(x2, y1),
                OVR::Vector3f(x2, y2, z1), OVR::Vector3f(x2, y2), OVR::Vector3f(x1, y2, z1), OVR::Vector3f(x1, y2),
                OVR::Vector3f(x1, y1, z2), OVR::Vector3f(x1, y1), OVR::Vector3f(x2, y1, z2), OVR::Vector3f(x2, y1),
                OVR::Vector3f(x2, y2, z2), OVR::Vector3f(x2, y2), OVR::Vector3f(x1, y2, z2), OVR::Vector3f(x1, y2)
            };

        GLushort CubeIndices[] =
            {
                0, 1, 3, 3, 1, 2,
                5, 4, 6, 6, 4, 7,
                8, 9, 11, 11, 9, 10,
                13, 12, 14, 14, 12, 15,
                16, 17, 19, 19, 17, 18,
                21, 20, 22, 22, 20, 23
            };

        for (int i = 0; i < sizeof(CubeIndices) / sizeof(CubeIndices[0]); ++i)
            AddIndex(CubeIndices[i] + GLushort(numVertices));

        // Generate a quad for each box face
        for (int v = 0; v < 6 * 4; v++)
        {
            // Make vertices, with some token lighting
            Vertex vvv; vvv.Pos = Vert[v][0]; vvv.U = Vert[v][1].x; vvv.V = Vert[v][1].y;
            float dist1 = (vvv.Pos - OVR::Vector3f(-2, 4, -2)).Length();
            float dist2 = (vvv.Pos - OVR::Vector3f(3, 4, -3)).Length();
            float dist3 = (vvv.Pos - OVR::Vector3f(-4, 3, 25)).Length();
            int   bri = rand() % 160;
            float B = ((c >> 16) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
            float G = ((c >>  8) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
            float R = ((c >>  0) & 0xff) * (bri + 192.0f * (0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
            vvv.C = (c & 0xff000000) +
                    ((R > 255 ? 255 : DWORD(R)) << 16) +
                    ((G > 255 ? 255 : DWORD(G)) << 8) +
                    (B > 255 ? 255 : DWORD(B));
            AddVertex(vvv);
        }
    }

    void Render(OVR::Matrix4f view, OVR::Matrix4f proj)
    {
        OVR::Matrix4f combined = proj * view * GetMatrix();

        glUseProgram(Fill->program);
        glUniform1i(glGetUniformLocation(Fill->program, "Texture0"), 0);
        glUniformMatrix4fv(glGetUniformLocation(Fill->program, "matWVP"), 1, GL_TRUE, (FLOAT*)&combined);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Fill->texture->texId);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->buffer);

        GLuint posLoc = glGetAttribLocation(Fill->program, "Position");
        GLuint colorLoc = glGetAttribLocation(Fill->program, "Color");
        GLuint uvLoc = glGetAttribLocation(Fill->program, "TexCoord");

        glEnableVertexAttribArray(posLoc);
        glEnableVertexAttribArray(colorLoc);
        glEnableVertexAttribArray(uvLoc);

        glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Pos));
        glVertexAttribPointer(colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex, C));
        glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, U));

        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, NULL);

        glDisableVertexAttribArray(posLoc);
        glDisableVertexAttribArray(colorLoc);
        glDisableVertexAttribArray(uvLoc);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glUseProgram(0);
    }
};

struct SkinnedMesh: protected QOpenGLExtraFunctions
{
    int     numModels;
    Model * Models[10];

    void    Add(Model * n)
    {
        Models[numModels++] = n;
    }

    void Render(OVR::Matrix4f view, OVR::Matrix4f proj)
    {
        for (int i = 0; i < numModels; ++i)
            Models[i]->Render(view, proj);
    }

    GLuint CreateShader(GLenum type, const GLchar* src)
    {
        GLuint shader = glCreateShader(type);

        glShaderSource(shader, 1, &src, NULL);
        glCompileShader(shader);

        GLint r;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &r);
        if (!r)
        {
            GLchar msg[1024];
            glGetShaderInfoLog(shader, sizeof(msg), 0, msg);
            if (msg[0]) {
                qDebug("Compiling shader failed: %s\n", msg);
            }
            return 0;
        }

        return shader;
    }

    void Init(int includeIntensiveGPUobject)
    {
        static const GLchar* VertexShaderSrc =
            "#version 150\n"
            "uniform mat4 matWVP;\n"
            "in      vec4 Position;\n"
            "in      vec4 Color;\n"
            "in      vec2 TexCoord;\n"
            "out     vec2 oTexCoord;\n"
            "out     vec4 oColor;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = (matWVP * Position);\n"
            "   oTexCoord   = TexCoord;\n"
            "   oColor.rgb  = pow(Color.rgb, vec3(2.2));\n"   // convert from sRGB to linear
            "   oColor.a    = Color.a;\n"
            "}\n";

        static const char* FragmentShaderSrc =
            "#version 150\n"
            "uniform sampler2D Texture0;\n"
            "in      vec4      oColor;\n"
            "in      vec2      oTexCoord;\n"
            "out     vec4      FragColor;\n"
            "void main()\n"
            "{\n"
            "   FragColor = oColor * texture2D(Texture0, oTexCoord);\n"
            "}\n";

        GLuint    vshader = CreateShader(GL_VERTEX_SHADER, VertexShaderSrc);
        GLuint    fshader = CreateShader(GL_FRAGMENT_SHADER, FragmentShaderSrc);

        // Make textures
        ShaderFill * grid_material[4];
        for (int k = 0; k < 4; ++k)
        {
            static DWORD tex_pixels[256 * 256];
            for (int j = 0; j < 256; ++j)
            {
                for (int i = 0; i < 256; ++i)
                {
                    if (k == 0) tex_pixels[j * 256 + i] = (((i >> 7) ^ (j >> 7)) & 1) ? 0xffb4b4b4 : 0xff505050;// floor
                    if (k == 1) tex_pixels[j * 256 + i] = (((j / 4 & 15) == 0) || (((i / 4 & 15) == 0) && ((((i / 4 & 31) == 0) ^ ((j / 4 >> 4) & 1)) == 0)))
                                                      ? 0xff3c3c3c : 0xffb4b4b4;// wall
                    if (k == 2) tex_pixels[j * 256 + i] = (i / 4 == 0 || j / 4 == 0) ? 0xff505050 : 0xffb4b4b4;// ceiling
                    if (k == 3) tex_pixels[j * 256 + i] = 0xffffffff;// blank
                }
            }
            TextureBuffer * generated_texture = new TextureBuffer(false, OVR::Sizei(256, 256), 4, (unsigned char *)tex_pixels);
            grid_material[k] = new ShaderFill(vshader, fshader, generated_texture);
        }

        glDeleteShader(vshader);
        glDeleteShader(fshader);

        // Construct geometry
        Model * m = new Model(OVR::Vector3f(0, 0, 0), grid_material[2]);  // Moving box
        m->AddSolidColorBox(0, 0, 0, +1.0f, +1.0f, 1.0f, 0xff404040);
        m->AllocateBuffers();
        Add(m);

        m = new Model(OVR::Vector3f(0, 0, 0), grid_material[1]);  // Walls
        m->AddSolidColorBox(-10.1f, 0.0f, -20.0f, -10.0f, 4.0f, 20.0f, 0xff808080); // Left Wall
        m->AddSolidColorBox(-10.0f, -0.1f, -20.1f, 10.0f, 4.0f, -20.0f, 0xff808080); // Back Wall
        m->AddSolidColorBox(10.0f, -0.1f, -20.0f, 10.1f, 4.0f, 20.0f, 0xff808080); // Right Wall
        m->AllocateBuffers();
        Add(m);

        if (includeIntensiveGPUobject)
        {
            m = new Model(OVR::Vector3f(0, 0, 0), grid_material[0]);  // Floors
            for (float depth = 0.0f; depth > -3.0f; depth -= 0.1f)
                m->AddSolidColorBox(9.0f, 0.5f, -depth, -9.0f, 3.5f, -depth, 0x10ff80ff); // Partition
            m->AllocateBuffers();
            Add(m);
        }

        m = new Model(OVR::Vector3f(0, 0, 0), grid_material[0]);  // Floors
        m->AddSolidColorBox(-10.0f, -0.1f, -20.0f, 10.0f, 0.0f, 20.1f, 0xff808080); // Main floor
        m->AddSolidColorBox(-15.0f, -6.1f, 18.0f, 15.0f, -6.0f, 30.0f, 0xff808080); // Bottom floor
        m->AllocateBuffers();
        Add(m);

        m = new Model(OVR::Vector3f(0, 0, 0), grid_material[2]);  // Ceiling
        m->AddSolidColorBox(-10.0f, 4.0f, -20.0f, 10.0f, 4.1f, 20.1f, 0xff808080);
        m->AllocateBuffers();
        Add(m);

        m = new Model(OVR::Vector3f(0, 0, 0), grid_material[3]);  // Fixtures & furniture
        m->AddSolidColorBox(9.5f, 0.75f, 3.0f, 10.1f, 2.5f, 3.1f, 0xff383838);   // Right side shelf// Verticals
        m->AddSolidColorBox(9.5f, 0.95f, 3.7f, 10.1f, 2.75f, 3.8f, 0xff383838);   // Right side shelf
        m->AddSolidColorBox(9.55f, 1.20f, 2.5f, 10.1f, 1.30f, 3.75f, 0xff383838); // Right side shelf// Horizontals
        m->AddSolidColorBox(9.55f, 2.00f, 3.05f, 10.1f, 2.10f, 4.2f, 0xff383838); // Right side shelf
        m->AddSolidColorBox(5.0f, 1.1f, 20.0f, 10.0f, 1.2f, 20.1f, 0xff383838);   // Right railing
        m->AddSolidColorBox(-10.0f, 1.1f, 20.0f, -5.0f, 1.2f, 20.1f, 0xff383838);   // Left railing
        for (float f = 5.0f; f <= 9.0f; f += 1.0f)
        {
            m->AddSolidColorBox(f, 0.0f, 20.0f, f + 0.1f, 1.1f, 20.1f, 0xff505050);// Left Bars
            m->AddSolidColorBox(-f, 1.1f, 20.0f, -f - 0.1f, 0.0f, 20.1f, 0xff505050);// Right Bars
        }
        m->AddSolidColorBox(-1.8f, 0.8f, 1.0f, 0.0f, 0.7f, 0.0f, 0xff505000); // Table
        m->AddSolidColorBox(-1.8f, 0.0f, 0.0f, -1.7f, 0.7f, 0.1f, 0xff505000); // Table Leg
        m->AddSolidColorBox(-1.8f, 0.7f, 1.0f, -1.7f, 0.0f, 0.9f, 0xff505000); // Table Leg
        m->AddSolidColorBox(0.0f, 0.0f, 1.0f, -0.1f, 0.7f, 0.9f, 0xff505000); // Table Leg
        m->AddSolidColorBox(0.0f, 0.7f, 0.0f, -0.1f, 0.0f, 0.1f, 0xff505000); // Table Leg
        m->AddSolidColorBox(-1.4f, 0.5f, -1.1f, -0.8f, 0.55f, -0.5f, 0xff202050); // Chair Set
        m->AddSolidColorBox(-1.4f, 0.0f, -1.1f, -1.34f, 1.0f, -1.04f, 0xff202050); // Chair Leg 1
        m->AddSolidColorBox(-1.4f, 0.5f, -0.5f, -1.34f, 0.0f, -0.56f, 0xff202050); // Chair Leg 2
        m->AddSolidColorBox(-0.8f, 0.0f, -0.5f, -0.86f, 0.5f, -0.56f, 0xff202050); // Chair Leg 2
        m->AddSolidColorBox(-0.8f, 1.0f, -1.1f, -0.86f, 0.0f, -1.04f, 0xff202050); // Chair Leg 2
        m->AddSolidColorBox(-1.4f, 0.97f, -1.05f, -0.8f, 0.92f, -1.10f, 0xff202050); // Chair Back high bar

        for (float f = 3.0f; f <= 6.6f; f += 0.4f)
            m->AddSolidColorBox(-3, 0.0f, f, -2.9f, 1.3f, f + 0.1f, 0xff404040); // Posts

        m->AllocateBuffers();
        Add(m);
    }

    SkinnedMesh() : numModels(0) {
        initializeOpenGLFunctions();
    }

    SkinnedMesh(bool includeIntensiveGPUobject) :
        numModels(0)
    {
        initializeOpenGLFunctions();
        Init(includeIntensiveGPUobject);
    }
    void Release()
    {
        while (numModels-- > 0)
            delete Models[numModels];
    }
    ~SkinnedMesh()
    {
        Release();
    }
};

VRRenderer::VRRenderer(QQuickWindow *window)
    : m_window(window)
{

    // Initializes LibOVR, and the Rift
    ovrInitParams initParams = { ovrInit_RequestVersion | ovrInit_FocusAware, OVR_MINOR_VERSION, NULL, 0, 0 };

    ovrResult result = ovr_Initialize(&initParams);
    VALIDATE(OVR_SUCCESS(result), "Failed to initialize libOVR.");

    QSGRendererInterface *rif = m_window->rendererInterface();
    VALIDATE(rif->graphicsApi() == QSGRendererInterface::OpenGL || rif->graphicsApi() == QSGRendererInterface::OpenGLRhi, "Only OpenGL is supported at this time.");

    initializeOpenGLFunctions();

#ifndef NDEBUG
    GL::DebugCallback::install(this);
#endif
}

VRRenderer::~VRRenderer()
{
}

void VRRenderer::init()
{
    if (m_fboId)
    {
        glGenFramebuffers(1, &m_fboId);
    }

    if (!session)
    {
        ovrResult result = ovr_Create(&session, &luid);
        if (!OVR_SUCCESS(result))
            return;

        if (Compare(luid, GetDefaultAdapterLuid())) // If luid that the Rift is on is not the default adapter LUID...
        {
            VALIDATE(false, "OpenGL supports only the default graphics adapter.");
        }

        ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);
        qDebug("ProductName\t = %s", hmdDesc.ProductName);
        qDebug("Manufacturer\t = %s", hmdDesc.Manufacturer);
        qDebug("SerialNumber\t = %s", hmdDesc.SerialNumber);

        // Setup Window and Graphics
        // Note: the mirror window can be any size, for this sample we use 1/2 the HMD resolution
        //ovrSizei windowSize = { hmdDesc.Resolution.w / 2, hmdDesc.Resolution.h / 2 };
        //m_window->resize(windowSize.w, windowSize.h);
        ovrSizei windowSize = { m_window->width(), m_window->height() };

        // Make eye render buffers
        for (int eye = 0; eye < 2; ++eye)
        {
            ovrSizei idealTextureSize = ovr_GetFovTextureSize(session, ovrEyeType(eye), hmdDesc.DefaultEyeFov[eye], 1);
            eyeRenderTexture[eye] = new OculusTextureBuffer(session, idealTextureSize, 1);

            if (!eyeRenderTexture[eye]->ColorTextureChain || !eyeRenderTexture[eye]->DepthTextureChain)
            {
                VALIDATE(false, "Failed to create texture.");
            }
        }

        ovrMirrorTextureDesc desc;
        memset(&desc, 0, sizeof(desc));
        desc.Width = windowSize.w;
        desc.Height = windowSize.h;
        desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;

        // Create mirror texture and an FBO used to copy mirror texture to back buffer
        result = ovr_CreateMirrorTextureWithOptionsGL(session, &desc, &mirrorTexture);
        if (!OVR_SUCCESS(result))
        {
            VALIDATE(false, "Failed to create mirror texture.");
        }

        // Configure the mirror read buffer
        GLuint texId;
        ovr_GetMirrorTextureBufferGL(session, mirrorTexture, &texId);

        glGenFramebuffers(1, &mirrorFBO);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
        glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

        // Make scene - can simplify further if needed
        roomScene = new SkinnedMesh(false);

        // FloorLevel will give tracking poses where the floor height is 0
        ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);

    }
}

void VRRenderer::paint()
{
    //qDebug() << "context swap interval =" << QOpenGLContext::currentContext()->format().swapInterval();

    // Play nice with the RHI. Not strictly needed when the scenegraph uses
    // OpenGL directly.
    m_window->beginExternalCommands();

    ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);
    ovrSizei windowSize = { m_window->width(), m_window->height() };

    ovrSessionStatus sessionStatus;
    ovr_GetSessionStatus(session, &sessionStatus);
    if (sessionStatus.ShouldQuit)
    {
        // Because the application is requested to quit, should not request retry
        return;
    }
    if (sessionStatus.ShouldRecenter)
        ovr_RecenterTrackingOrigin(session);

    if (sessionStatus.IsVisible)
    {
        // Animate the cube
        static float cubeClock = 0;
        if (sessionStatus.HasInputFocus) // Pause the application if we are not supposed to have input.
            roomScene->Models[0]->Pos = OVR::Vector3f(9 * (float)sin(cubeClock), 3, 9 * (float)cos(cubeClock += 0.015f));

        // Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyePose) may change at runtime.
        ovrEyeRenderDesc eyeRenderDesc[2];
        eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
        eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

        // Get eye poses, feeding in correct IPD offset
        ovrPosef EyeRenderPose[2];
        ovrPosef HmdToEyePose[2] = { eyeRenderDesc[0].HmdToEyePose,
                                    eyeRenderDesc[1].HmdToEyePose};

        double sensorSampleTime;    // sensorSampleTime is fed into the layer later
        ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyePose, EyeRenderPose, &sensorSampleTime);

        ovrTimewarpProjectionDesc posTimewarpProjectionDesc = {};

        // Render Scene to Eye Buffers
        for (int eye = 0; eye < 2; ++eye)
        {
            // Switch to eye render target
            eyeRenderTexture[eye]->SetAndClearRenderSurface();

            // Get view and projection matrices
            OVR::Matrix4f rollPitchYaw = OVR::Matrix4f(OVR::Quatf(Orientation.x(), Orientation.y(), Orientation.z(), Orientation.scalar()));
            OVR::Matrix4f finalRollPitchYaw = rollPitchYaw * OVR::Matrix4f(EyeRenderPose[eye].Orientation);
            OVR::Vector3f finalUp = finalRollPitchYaw.Transform(OVR::Vector3f(0, 1, 0));
            OVR::Vector3f finalForward = finalRollPitchYaw.Transform(OVR::Vector3f(0, 0, -1));
            OVR::Vector3f shiftedEyePos = OVR::Vector3f(Position.x(), Position.y(), Position.z()) + rollPitchYaw.Transform(EyeRenderPose[eye].Position);

            OVR::Matrix4f view = OVR::Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
            OVR::Matrix4f proj = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], 0.2f, 1000.0f, ovrProjection_None);
            posTimewarpProjectionDesc = ovrTimewarpProjectionDesc_FromProjection(proj, ovrProjection_None);

            // Render world
            roomScene->Render(view, proj);

            // Avoids an error when calling SetAndClearRenderSurface during next iteration.
            // Without this, during the next while loop iteration SetAndClearRenderSurface
            // would bind a framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
            // associated with COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
            eyeRenderTexture[eye]->UnsetRenderSurface();

            // Commit changes to the textures so they get picked up frame
            eyeRenderTexture[eye]->Commit();
        }

        // Do distortion rendering, Present and flush/sync

        ovrLayerEyeFovDepth ld = {};
        ld.Header.Type  = ovrLayerType_EyeFovDepth;
        ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.
        ld.ProjectionDesc = posTimewarpProjectionDesc;
        ld.SensorSampleTime = sensorSampleTime;

        for (int eye = 0; eye < 2; ++eye)
        {
            ld.ColorTexture[eye] = eyeRenderTexture[eye]->ColorTextureChain;
            ld.DepthTexture[eye] = eyeRenderTexture[eye]->DepthTextureChain;
            ld.Viewport[eye]     = OVR::Recti(eyeRenderTexture[eye]->GetSize());
            ld.Fov[eye]          = hmdDesc.DefaultEyeFov[eye];
            ld.RenderPose[eye]   = EyeRenderPose[eye];
        }

        ovrLayerHeader* layers = &ld.Header;
        ovrResult result = ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1);
        // exit the rendering loop if submit returns an error, will retry on ovrError_DisplayLost
        if (!OVR_SUCCESS(result))
            return;

        frameIndex++;
    }

    // Blit mirror texture to back buffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    GLint w = windowSize.w;
    GLint h = windowSize.h;
    glBlitFramebuffer(0, h, w, 0,
                      0, 0, w, h,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // Not strictly needed for this example, but generally useful for when
    // mixing with raw OpenGL.
    m_window->resetOpenGLState();

    m_window->endExternalCommands();

    // Keep the scene tree dirty so that it renders at every frame. It's not a
    // UI. It's a VR app. We don't need this CPU usage optimization.
    m_window->update();
}

void VRRenderer::cleanup()
{
    if (roomScene)
    {
        delete roomScene;
        roomScene = nullptr;
    }

    if (mirrorFBO) {
        glDeleteFramebuffers(1, &mirrorFBO);
        mirrorFBO = 0;
    }

    if (mirrorTexture)
    {
        ovr_DestroyMirrorTexture(session, mirrorTexture);
        mirrorTexture = nullptr;
    }
    for (int eye = 0; eye < 2; ++eye)
    {
        delete eyeRenderTexture[eye];
        eyeRenderTexture[eye] = nullptr;
    }

    if (m_fboId)
    {
        glDeleteFramebuffers(1, &m_fboId);
        m_fboId = 0;
    }

    ovr_Destroy(session);
    session = nullptr;
}


