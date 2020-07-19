#ifndef DEVICE_H
#define DEVICE_H

#include <QElapsedTimer>
#include <QOpenGLExtraFunctions>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#ifdef HAVE_LIBOVR
#include <OVR_CAPI_GL.h>
#endif


#include "Headset.h"
#include "Node.h"
#include "VRTypes.h"

struct RoomScene;
struct OculusTextureBuffer;

class Device : public Node, public QOpenGLExtraFunctions
{
    Q_OBJECT

    Q_PROPERTY(Headset* headset READ headset WRITE setHeadset NOTIFY headsetChanged)

public:

    explicit Device(Node *parent = nullptr);

    Headset *headset() const { return m_headset; }
    void setHeadset(Headset *newHeadset);

#ifdef HAVE_LIBOVR
    ovrSession session() const { return m_session; }
    const ovrGraphicsLuid &luid() const { return m_luid; }
#endif

    float t() const { return m_t; }
    float dt() const { return m_dt; }

    const glm::mat4 &view() const { return m_view; }
    const glm::mat4 &proj() const { return m_proj; }
    const glm::vec3 &eyePosition() const { return m_eyePosition; }
    const glm::quat &eyeOrientation() const { return m_eyeOrientation; }

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

    glm::mat4 m_view;
    glm::mat4 m_proj;
    glm::vec3 m_eyePosition;
    glm::quat m_eyeOrientation;

    Headset *m_headset;
    Headset *m_defaultHeadset;

    int64_t m_frameIndex = 0;

    QElapsedTimer m_runningTimer;

    RoomScene* m_roomScene = nullptr;
#ifdef HAVE_LIBOVR
    ovrSession m_session = nullptr;
    ovrGraphicsLuid m_luid = {};
    OculusTextureBuffer *m_eyeRenderTexture[2] = {};
    ovrMirrorTexture m_mirrorTexture = nullptr;
    GLuint m_mirrorFBO = 0;
#endif
};

#endif // DEVICE_H
