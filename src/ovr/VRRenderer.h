#ifndef VRRENDERER_H
#define VRRENDERER_H

#include <QtQuick/QQuickWindow>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLExtraFunctions>

#include <OVR_CAPI_GL.h>
#include <Extras/OVR_Math.h>

struct OculusTextureBuffer;
struct Scene;

class VRRenderer : public QObject, protected QOpenGLExtraFunctions
{
    Q_OBJECT
public:
    VRRenderer(QQuickWindow *window);
    ~VRRenderer();

    QQuickWindow *window() const { return m_window;}

    void setViewportSize(const QSize &size) { m_viewportSize = size; }

    static void APIENTRY DebugGLCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

public slots:
    void init();
    void paint();
    void cleanup();

private:
    QSize m_viewportSize;
    QQuickWindow *m_window;
    GLuint m_fboId = 0;

    OculusTextureBuffer * eyeRenderTexture[2] = { nullptr, nullptr };
    ovrMirrorTexture mirrorTexture = nullptr;
    GLuint          mirrorFBO = 0;
    Scene         * roomScene = nullptr;
    long long frameIndex = 0;

    ovrSession session = nullptr;
    ovrGraphicsLuid luid = {};

    float Yaw = 3.141592f;
    OVR::Vector3f Pos2 = OVR::Vector3f(0.0f, 0.0f, -5.0f);
};

#endif // VRRENDERER_H
