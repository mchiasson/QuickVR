#ifndef TEXTURE_H
#define TEXTURE_H

#include "Device.h"

#include <QtConcurrent>
#include <QAtomicInteger>
#include <QMutex>

class Texture : public Node
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(Target  target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(Format  format READ format WRITE setFormat NOTIFY formatChanged)
    Q_PROPERTY(Filter  filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(Wrap    wrapS  READ wrapS  WRITE setWrapS  NOTIFY wrapSChanged)
    Q_PROPERTY(Wrap    wrapT  READ wrapT  WRITE setWrapT  NOTIFY wrapTChanged)
    Q_PROPERTY(Status  status READ status                 NOTIFY statusChanged)

public:

    enum Target
    {
        TargetTexture2D = GL_TEXTURE_2D,
        TargetTextureCubeMap = GL_TEXTURE_CUBE_MAP,
    };
    Q_ENUMS(Target)

    enum Format
    {
        FormatGrey = GL_RED,
        FormatGreyAlpha = GL_RG,
        FormatRGB = GL_RGB,
        FormatRGBA = GL_RGBA
    };
    Q_ENUMS(Format)

    enum Filter
    {
        FilterNone,
        FilterBilinear,
        FilterTrilinear,
        FilterAnisotropic
    };
    Q_ENUMS(Filter)

    enum Status
    {
        StatusNone,
        StatusLoading,
        StatusProcessing,
        StatusReady,
        StatusError
    };
    Q_ENUMS(Status)

    enum Wrap
    {
        WrapRepeat = GL_REPEAT,
        WrapClampToEdge = GL_CLAMP_TO_EDGE
    };
    Q_ENUMS(Wrap)

    enum DirtyFlag
    {
        DirtyNone   = 0x00,
        DirtySource = 0x01,
        DirtyTarget = 0x02,
        DirtyFormat = 0x04,
        DirtyFilter = 0x08,
        DirtyWrapS  = 0x10,
        DirtyWrapT  = 0x20,
        DirtyBitmap = 0x40
    };

    struct Bitmap
    {
        ~Bitmap();
        int width = 0;
        int height = 0;
        int channelCount = 0;
        unsigned char *decodedData = nullptr;
    };

    explicit Texture(Node *parent = nullptr);

    QString source() const { return m_source; }
    void setSource(const QString &newSource);

    Target target() const { return m_target; }
    void setTarget(Target newFormat);

    Format format() const { return m_format; }
    void setFormat(Format newFormat);

    Filter filter() const { return m_filter; }
    void setFilter(Filter newFilter);

    Wrap wrapS() const { return m_wrapS; }
    void setWrapS(Wrap newWrap);

    Wrap wrapT() const { return m_wrapT; }
    void setWrapT(Wrap newWrap);

    Status status() const { return m_status; }
    void setStatus(Status newStatus);

    GLuint handle() const { return m_handle; }

    void bind(int slot);

signals:
    void sourceChanged(const QString &newSource);
    void targetChanged(Target newTarget);
    void formatChanged(Format newFormat);
    void filterChanged(Filter newFilter);
    void wrapSChanged(Wrap newWrap);
    void wrapTChanged(Wrap newWrap);
    void statusChanged(Status newStatus);

protected:
    void onImageDecodingComplete();
    virtual void onUpdate() override;
    virtual void onShutdown() override;

protected:
    GLuint    m_handle = 0;
    QString   m_source;
    Format    m_format = FormatRGBA;
    Status    m_status = StatusNone;
    Target    m_target = TargetTexture2D;
    Filter    m_filter = FilterAnisotropic;
    Wrap      m_wrapS  = WrapClampToEdge;
    Wrap      m_wrapT  = WrapClampToEdge;
    QAtomicInteger<int> m_dirtyFlag = (DirtyFilter|DirtyWrapS|DirtyWrapT);
    QFutureWatcher<QSharedPointer<Bitmap>> m_dataFutureWatcher;
    QMutex    m_mutex;
};

#endif // TEXTURE_H
