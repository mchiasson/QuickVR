#ifndef TEXTURE_H
#define TEXTURE_H

#include "Device.h"

#include <QtConcurrent>

class Texture : public Node
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(Target target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(Format format READ format WRITE setFormat NOTIFY formatChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

public:

    enum Target
    {
        TargetTexture2D      = 0,
        TargetTextureCubeMap = 1
    };
    Q_ENUMS(Target)

    enum Format
    {
        FormatDefault   = 0,
        FormatGrey      = 1,
        FormatGreyAlpha = 2,
        FormatRGB       = 3,
        FormatRGBA      = 4
    };
    Q_ENUMS(Format)

    enum Status
    {
        StatusNone = 0,
        StatusLoading,
        StatusProcessing,
        StatusReady,
        StatusError
    };
    Q_ENUMS(Status)

    struct Bitmap
    {
        ~Bitmap();
        GLenum  target = TargetTexture2D;
        int width = 0, height = 0, channelCount = 0;
        unsigned char *decodedData = nullptr;
    };

    explicit Texture(Node *parent = nullptr);

    QString source() const { return m_source; }
    void setSource(const QString &newSource);

    Target target() const { return m_target; }
    void setTarget(Target newFormat);

    Format format() const { return m_format; }
    void setFormat(Format newFormat);

    Status status() const { return m_status; }
    void setStatus(Status newStatus);

    GLuint handle() const { return m_handle; }

    void bind(int slot) const;
    void setSampleMode(int);

signals:

    void sourceChanged(const QString &newSource);
    void targetChanged(Target newTarget);
    void formatChanged(Format newFormat);
    void statusChanged(Status newStatus);

protected slots:

    void loadImage();

protected:

    void onImageDecodingComplete();
    virtual void onUpdate() override;
    virtual void onShutdown() override;

protected:
    QString m_source;
    Format m_format = FormatDefault;
    Status m_status = StatusNone;
    Target  m_target = TargetTexture2D;
    GLuint  m_handle = 0;
    QFutureWatcher<QSharedPointer<Bitmap>> m_dataFutureWatcher;
};

#endif // TEXTURE_H
