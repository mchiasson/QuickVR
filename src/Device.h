#ifndef DEVICE_H
#define DEVICE_H

#include <QOpenGLExtraFunctions>
#include <OVR_CAPI_GL.h>
#include <QtGui/QVector3D>
#include <QtGui/QQuaternion>
#include <QElapsedTimer>
#include <Headset.h>

#include "Node.h"
#include "VRTypes.h"

#include <Extras/OVR_Math.h>

struct OculusTextureBuffer;

class Device : public Node, public QOpenGLExtraFunctions
{
    Q_OBJECT

    Q_PROPERTY(Headset* headset READ headset WRITE setHeadset NOTIFY headsetChanged)

public:

    explicit Device(Node *parent = nullptr);

    Headset *headset() const { return m_headset; }
    void setHeadset(Headset *newHeadset);

    ovrSession session() const { return m_session; }
    const ovrGraphicsLuid &luid() const { return m_luid; }

    float t() const { return m_t; }
    float dt() const { return m_dt; }

    const OVR::Matrix4f &view() const { return m_view; }
    const OVR::Matrix4f &proj() const { return m_proj; }

signals:

    void init();
    void update();
    void render();
    void shutdown();

    void headsetChanged(Headset *newHeadset);

protected slots:

    void onSceneGraphInitialized();
    void onBeforeRenderPassRecording();
    void onSceneGraphAboutToStop();

protected:

    float m_t = 0.0f;
    float m_dt = 0.0f;

    OVR::Matrix4f m_view;
    OVR::Matrix4f m_proj;

    Headset *m_headset;
    Headset *m_defaultHeadset;
    ovrSession m_session = nullptr;
    ovrGraphicsLuid m_luid = {};

    OculusTextureBuffer *m_eyeRenderTexture[2] = {};
    ovrMirrorTexture m_mirrorTexture = nullptr;
    GLuint m_mirrorFBO = 0;
    int64_t m_frameIndex = 0;

    QElapsedTimer m_runningTimer;
};

#endif // DEVICE_H
