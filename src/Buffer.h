#ifndef BUFFER_H
#define BUFFER_H

#include <QOpenGLExtraFunctions>

#include "Node.h"

#include <QMutex>

class Buffer : public Node
{
    Q_OBJECT
    Q_PROPERTY(Target     target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(Usage      usage  READ usage  WRITE setUsage  NOTIFY usageChanged)
    Q_PROPERTY(QByteArray data   READ data   WRITE setData   NOTIFY dataChanged)

public:

    enum Target
    {
        TargetIndexBuffer   = GL_ELEMENT_ARRAY_BUFFER,
        TargetUniformBuffer = GL_UNIFORM_BUFFER,
        TargetVertexBuffer  = GL_ARRAY_BUFFER
    };
    Q_ENUM(Target)

    enum Usage
    {
        UsageDynamicDraw = GL_DYNAMIC_DRAW,
        UsageStaticDraw  = GL_STATIC_DRAW,
        UsageStreamDraw  = GL_STREAM_DRAW
    };
    Q_ENUM(Usage)

    enum DirtyFlag
    {
        DirtyNone   = 0x00,
        DirtyTarget = 0x01,
        DirtyUsage  = 0x02,
        DirtyData   = 0x04,
    };

    explicit Buffer(Node *parent = nullptr) : Node(parent) {}

    Target target() const { return m_target; }
    void setTarget(Target newTarget);

    Usage usage() const { return m_usage; }
    void setUsage(Usage newUsage);

    const QByteArray &data() const { return m_data; }
    void setData(const QByteArray &newData);

    void bind();

    GLuint handle() const { return m_handle; }

signals:

    void targetChanged(Target newTarget);
    void usageChanged(Usage newUsage);
    void dataChanged(const QByteArray &newData);

protected:

    virtual void onUpdate() override;
    virtual void onShutdown() override;

protected:

    GLuint     m_handle = 0;
    Target     m_target = TargetVertexBuffer;
    Usage      m_usage  = UsageStaticDraw;
    QByteArray m_data;

    QAtomicInteger<int> m_dirtyFlag = 0;
    QMutex    m_mutex;

};

#endif // BUFFER_H
