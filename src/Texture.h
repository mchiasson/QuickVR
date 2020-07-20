#ifndef TEXTURE_H
#define TEXTURE_H

#include "Device.h"

#include <QtConcurrent>

class Texture : public Node
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

public:
    enum Status
    {
        None = 0,
        Loading,
        Processing,
        Ready,
        Error
    };
    Q_ENUMS(Status)

    struct Bitmap
    {
        ~Bitmap();
        int width = 0, height = 0, channelCount = 0;
        unsigned char *data = nullptr;
    };

    explicit Texture(Node *parent = nullptr);

    QString source() const { return m_source; }
    void setSource(QString newSource);

    Status status() const { return m_status; }
    void setStatus(Status newStatus);

    GLenum target() const { return m_target; }
    GLuint handle() const { return m_handle; }

    void bind(int slot) const;
    void setSampleMode(int);

signals:

    void sourceChanged(const QString &newSource);
    void statusChanged(Status newStatus);

protected:

    void onImageDecodingComplete();
    virtual void onUpdate() override;
    virtual void onShutdown() override;

protected:
    QString m_source;
    Status m_status = None;
    GLenum  m_target = 0;
    GLuint  m_handle = 0;
    QFutureWatcher<QSharedPointer<Bitmap>> m_dataFutureWatcher;
};

#endif // TEXTURE_H
