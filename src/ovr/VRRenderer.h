#ifndef VRRENDERER_H
#define VRRENDERER_H

#include <QtQuick/QQuickWindow>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLExtraFunctions>
#include <QtGui/QVector3D>

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

    static void APIENTRY DebugGLCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

public slots:
    void init();
    void paint();
    void cleanup();

private:
    QQuickWindow *m_window;
    GLuint m_fboId = 0;

    OculusTextureBuffer * eyeRenderTexture[2] = { nullptr, nullptr };
    ovrMirrorTexture mirrorTexture = nullptr;
    GLuint          mirrorFBO = 0;
    Scene         * roomScene = nullptr;
    long long frameIndex = 0;

    ovrSession session = nullptr;
    ovrGraphicsLuid luid = {};

public:
    QQuaternion Orientation;
    QVector3D Position;
};

#endif // VRRENDERER_H
