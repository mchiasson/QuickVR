#include "Device.h"

#include <QDebug>
#include <QQuickWindow>
#include <QGuiApplication>
#include <QtMath>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#ifdef HAVE_LIBOVR
#include <Extras/OVR_Math.h>
#endif

#include "DebugCallback.h"
#include "RoomScene.h"

#if defined(_WIN32)
#include <dxgi.h> // for GetDefaultAdapterLuid
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "user32.lib")
#endif

#ifdef HAVE_LIBOVR
#ifndef VALIDATE
#define VALIDATE(x, msg) if (!(x)) { MessageBoxA(nullptr, (msg), qPrintable(qApp->applicationName()), MB_ICONERROR | MB_OK); exit(-1); }
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


static void OVR_CDECL LogCallback(uintptr_t /*userData*/, int /*level*/, const char* message)
{
    qDebug() << message;
}

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
#endif

Device::Device(Node *parent) : Node(parent)
{
    setObjectName("Device");
    m_device = this;

    m_defaultHeadset = new Headset(this);
    m_headset = m_defaultHeadset;

    connect(this, &QQuickItem::windowChanged,
            [&](QQuickWindow* window) {
        if (window) {
            direct_connect(window, &QQuickWindow::sceneGraphInitialized, this, &Device::onSceneGraphInitialized);
            direct_connect(window, &QQuickWindow::beforeRenderPassRecording, this, &Device::onBeforeRenderPassRecording);
            direct_connect(window, &QQuickWindow::sceneGraphAboutToStop, this, &Device::onSceneGraphAboutToStop);
        }
    });
}

void Device::setHeadset(Headset *newHeadset)
{
    if (m_headset != newHeadset)
    {
        if (newHeadset == nullptr)
        {
            m_headset = m_defaultHeadset;
        }
        else
        {
            m_headset = newHeadset;
        }
        emit headsetChanged(newHeadset);
    }
}

void Device::onSceneGraphInitialized()
{
    initializeOpenGLFunctions();

#ifndef NDEBUG
    GL::DebugCallback::install(this);
#endif

#ifdef HAVE_LIBOVR
    // Initializes LibOVR, and the Rift
    ovrInitParams initParams = { ovrInit_RequestVersion | ovrInit_FocusAware, OVR_MINOR_VERSION, LogCallback, 0, 0 };

    ovrResult result = ovr_Initialize(&initParams);
    VALIDATE(OVR_SUCCESS(result), "Failed to initialize libOVR.");

    QSGRendererInterface *rif = window()->rendererInterface();
    VALIDATE(rif->graphicsApi() == QSGRendererInterface::OpenGL || rif->graphicsApi() == QSGRendererInterface::OpenGLRhi, "Only OpenGL is supported at this time.");

    result = ovr_Create(&m_session, &m_luid);
    VALIDATE(OVR_SUCCESS(result), "unable to create session.")

            if (Compare(m_luid, GetDefaultAdapterLuid())) // If luid that the Rift is on is not the default adapter LUID...
    {
        VALIDATE(false, "OpenGL supports only the default graphics adapter.");
    }

    ovrHmdDesc hmdDesc = ovr_GetHmdDesc(m_session);
    qDebug("ProductName\t = %s", hmdDesc.ProductName);
    qDebug("Manufacturer\t = %s", hmdDesc.Manufacturer);
    qDebug("SerialNumber\t = %s", hmdDesc.SerialNumber);

    // Make eye render buffers
    for (int eye = 0; eye < 2; ++eye)
    {
        ovrSizei idealTextureSize = ovr_GetFovTextureSize(m_session, ovrEyeType(eye), hmdDesc.DefaultEyeFov[eye], 1);
        m_eyeRenderTexture[eye] = new OculusTextureBuffer(m_session, idealTextureSize, 1);

        if (!m_eyeRenderTexture[eye]->ColorTextureChain || !m_eyeRenderTexture[eye]->DepthTextureChain)
        {
            VALIDATE(false, "Failed to create texture.");
        }
    }

    ovrMirrorTextureDesc desc;
    memset(&desc, 0, sizeof(desc));
    desc.Width = window()->width();
    desc.Height = window()->height();
    desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.MirrorOptions = ovrMirrorOption_PostDistortion;

    // Create mirror texture and an FBO used to copy mirror texture to back buffer
    result = ovr_CreateMirrorTextureWithOptionsGL(m_session, &desc, &m_mirrorTexture);
    if (!OVR_SUCCESS(result))
    {
        VALIDATE(false, "Failed to create mirror texture.");
    }

    // Configure the mirror read buffer
    GLuint texId;
    ovr_GetMirrorTextureBufferGL(m_session, m_mirrorTexture, &texId);

    glGenFramebuffers(1, &m_mirrorFBO);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_mirrorFBO);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
    glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // FloorLevel will give tracking poses where the floor height is 0
    ovr_SetTrackingOriginType(m_session, ovrTrackingOrigin_FloorLevel);

#endif
    m_roomScene = new RoomScene(false);

    emit init();

    m_runningTimer.start();
}

void Device::onBeforeRenderPassRecording()
{
    // Play nice with the RHI. Not strictly needed when the scenegraph uses
    // OpenGL directly.
    window()->beginExternalCommands();

    static float prev_t = 0;
    m_t =  m_runningTimer.elapsed() / 1000.0f;
    m_dt = m_t - prev_t;
    prev_t = m_t;

    // Animate the cube
    static float cubeClock = 0;
    m_roomScene->Models[0]->Pos = glm::vec3(9 * (float)sin(cubeClock), 3, 9 * (float)cos(cubeClock += 0.015f));

    // Update world
    emit update();


#ifdef HAVE_LIBOVR
    ovrHmdDesc hmdDesc = ovr_GetHmdDesc(m_session);

    ovrSessionStatus sessionStatus;
    ovr_GetSessionStatus(m_session, &sessionStatus);
    if (sessionStatus.ShouldQuit)
    {
        // Because the application is requested to quit, should not request retry
        qApp->quit();
        return;
    }

    if (sessionStatus.ShouldRecenter) ovr_RecenterTrackingOrigin(m_session);

    if (sessionStatus.IsVisible)
    {
        // Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyePose) may change at runtime.
        ovrEyeRenderDesc eyeRenderDesc[2];
        eyeRenderDesc[0] = ovr_GetRenderDesc(m_session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
        eyeRenderDesc[1] = ovr_GetRenderDesc(m_session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

        // Get eye poses, feeding in correct IPD offset
        ovrPosef EyeRenderPose[2];
        ovrPosef HmdToEyePose[2] = { eyeRenderDesc[0].HmdToEyePose,
                                     eyeRenderDesc[1].HmdToEyePose};

        double sensorSampleTime;    // sensorSampleTime is fed into the layer later
        ovr_GetEyePoses(m_session, m_frameIndex, ovrTrue, HmdToEyePose, EyeRenderPose, &sensorSampleTime);

        ovrTimewarpProjectionDesc posTimewarpProjectionDesc = {};

        glm::vec3 camPosition    = glm::vec3(m_headset->x(), m_headset->y(), m_headset->z());
        glm::quat camOrientation = glm::angleAxis(glm::radians((float)m_headset->rotation()), glm::vec3(0.0f, 1.0f, 0.0f));

        // Render Scene to Eye Buffers
        for (int eye = 0; eye < 2; ++eye)
        {
            // Switch to eye render target
            m_eyeRenderTexture[eye]->SetAndClearRenderSurface();
            m_eyeOrientation = camOrientation * glm::quat(EyeRenderPose[eye].Orientation.w, EyeRenderPose[eye].Orientation.x, EyeRenderPose[eye].Orientation.y, EyeRenderPose[eye].Orientation.z);

            // Get view and projection matrices
            glm::vec3 up     = m_eyeOrientation * glm::vec3(0,  1,  0);
            glm::vec3 center = m_eyeOrientation * glm::vec3(0,  0, -1);
            m_eyePosition = camPosition + camOrientation * glm::vec3(EyeRenderPose[eye].Position.x, EyeRenderPose[eye].Position.y, EyeRenderPose[eye].Position.z);

            m_view = glm::lookAt(m_eyePosition, m_eyePosition + center, up);

            // TODO replace by glm::perspective once we know how to get the FOV angle out of hmdDesc.DefaultEyeFov[eye]. This would eliminiate the need of converting from row major OVR::Matrix4f to column major glm::mat4
            OVR::Matrix4f proj = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], 0.2f, 1000.0f, ovrProjection_None);
            posTimewarpProjectionDesc = ovrTimewarpProjectionDesc_FromProjection(proj, ovrProjection_None);

            // convert from row major OVR::Matrix4f to column major glm::mat4
            m_proj = glm::mat4(
                proj.M[0][0], proj.M[1][0], proj.M[2][0], proj.M[3][0],
                proj.M[0][1], proj.M[1][1], proj.M[2][1], proj.M[3][1],
                proj.M[0][2], proj.M[1][2], proj.M[2][2], proj.M[3][2],
                proj.M[0][3], proj.M[1][3], proj.M[2][3], proj.M[3][3]);

            // Render world
            m_roomScene->Render(m_view, m_proj);
            emit render();

            // Avoids an error when calling SetAndClearRenderSurface during next iteration.
            // Without this, during the next while loop iteration SetAndClearRenderSurface
            // would bind a framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
            // associated with COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
            m_eyeRenderTexture[eye]->UnsetRenderSurface();

            // Commit changes to the textures so they get picked up frame
            m_eyeRenderTexture[eye]->Commit();
        }

        // Do distortion rendering, Present and flush/sync

        ovrLayerEyeFovDepth ld = {};
        ld.Header.Type  = ovrLayerType_EyeFovDepth;
        ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.
        ld.ProjectionDesc = posTimewarpProjectionDesc;
        ld.SensorSampleTime = sensorSampleTime;

        for (int eye = 0; eye < 2; ++eye)
        {
            ld.ColorTexture[eye] = m_eyeRenderTexture[eye]->ColorTextureChain;
            ld.DepthTexture[eye] = m_eyeRenderTexture[eye]->DepthTextureChain;
            ld.Viewport[eye]     = OVR::Recti(m_eyeRenderTexture[eye]->GetSize());
            ld.Fov[eye]          = hmdDesc.DefaultEyeFov[eye];
            ld.RenderPose[eye]   = EyeRenderPose[eye];
        }

        ovrLayerHeader* layers = &ld.Header;
        ovrResult result = ovr_SubmitFrame(m_session, m_frameIndex, nullptr, &layers, 1);
        // exit the rendering loop if submit returns an error, will retry on ovrError_DisplayLost
        if (!OVR_SUCCESS(result))
            return;

        m_frameIndex++;
    }

    // Blit mirror texture to back buffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_mirrorFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    GLint w = window()->width();
    GLint h = window()->height();
    glBlitFramebuffer(0, h, w, 0,
                      0, 0, w, h,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
#else


    m_eyePosition.x = m_headset->x();
    m_eyePosition.y = m_headset->y()+2;
    m_eyePosition.z = m_headset->z();
    m_eyeOrientation = glm::angleAxis(glm::radians((float)m_headset->rotation()), glm::vec3(0.0f, 1.0f, 0.0f));

    // Get view and projection matrices
    glm::vec3 up = m_eyeOrientation * glm::vec3(0, 1, 0);
    glm::vec3 forward = m_eyeOrientation * glm::vec3(0, 0, -1);
    m_view = glm::lookAt(m_eyePosition, m_eyePosition + forward, up);
    m_proj = glm::perspective(glm::radians(45.0f), (float)width() / (float)height(), 0.2f, 1000.0f);

    // Render world
    m_roomScene->Render(m_view, m_proj);
    emit render();

#endif

    // Not strictly needed for this example, but generally useful for when
    // mixing with raw OpenGL.
    window()->resetOpenGLState();

    window()->endExternalCommands();

    // Keep the scene tree dirty so that it renders at every frame. It's not a
    // UI. It's a VR app. We don't need this CPU usage optimization.
    window()->update();
}

void Device::onSceneGraphAboutToStop()
{
    emit shutdown();

    delete m_roomScene;
#ifdef HAVE_LIBOVR
    if (m_mirrorFBO) glDeleteFramebuffers(1, &m_mirrorFBO);
    if (m_mirrorTexture) ovr_DestroyMirrorTexture(m_session, m_mirrorTexture);
    for (int eye = 0; eye < 2; ++eye)
    {
        delete m_eyeRenderTexture[eye];
    }
    ovr_Destroy(m_session);
#endif
}
