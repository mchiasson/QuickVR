#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <QDebug>

Texture::Bitmap::~Bitmap()
{
    if (decodedData)
    {
        stbi_image_free(decodedData);
        decodedData = nullptr;
    }
}

Texture::Texture(Node *parent) : Node(parent)
{
    connect(this, SIGNAL(sourceChanged), this, SLOT(loadImage), Qt::QueuedConnection);
    connect(this, SIGNAL(targetChanged), this, SLOT(loadImage), Qt::QueuedConnection);
    connect(this, SIGNAL(formatChanged), this, SLOT(loadImage), Qt::QueuedConnection);
    connect(&m_dataFutureWatcher, &QFutureWatcher<QByteArray>::finished, this, &Texture::onImageDecodingComplete);
}

void Texture::setSource(const QString &newSource)
{
    if (m_source != newSource)
    {
        m_source = newSource;
        emit sourceChanged(newSource);
    }

}

void Texture::setTarget(Target newTarget)
{
    if (m_target != newTarget)
    {
        m_target = newTarget;
        emit targetChanged(newTarget);
    }
}

void Texture::setFormat(Format newFormat)
{
    if (m_format != newFormat)
    {
        m_format = newFormat;
        emit formatChanged(newFormat);
    }
}

void Texture::setStatus(Status newStatus)
{
    if (m_status != newStatus)
    {
        m_status = newStatus;
        emit statusChanged(newStatus);
    }
}


void Texture::bind(int slot) const
{
    if (handle())
    {
        device()->glActiveTexture(GL_TEXTURE0 + slot);
        device()->glBindTexture(target(), handle());
    }
}

void Texture::setSampleMode(int sm)
{
    if (handle() == 0) return;

    bind(0);

    switch (sm & Sample_FilterMask)
    {
    case Sample_Linear:
        device()->glTexParameteri(target(), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        device()->glTexParameteri(target(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        if(GL_EXT_texture_filter_anisotropic)
            device()->glTexParameteri(target(), GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
        break;

    case Sample_Anisotropic:
        device()->glTexParameteri(target(), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        device()->glTexParameteri(target(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        if(GL_EXT_texture_filter_anisotropic)
            device()->glTexParameteri(target(), GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);
        break;

    case Sample_Nearest:
        device()->glTexParameteri(target(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        device()->glTexParameteri(target(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        if(GL_EXT_texture_filter_anisotropic)
            device()->glTexParameteri(target(), GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
        break;
    }

    switch (sm & Sample_AddressMask)
    {
    case Sample_Repeat:
        device()->glTexParameteri(target(), GL_TEXTURE_WRAP_S, GL_REPEAT);
        device()->glTexParameteri(target(), GL_TEXTURE_WRAP_T, GL_REPEAT);
        break;

    case Sample_Clamp:
        device()->glTexParameteri(target(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        device()->glTexParameteri(target(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        break;

    case Sample_ClampBorder:
        device()->glTexParameteri(target(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        device()->glTexParameteri(target(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        break;
    }
}

void Texture::loadImage() {
    if (status() != StatusLoading)
    {
        setStatus(StatusLoading);
        // Load the scene from a thread in Qt's thread pool.
        m_dataFutureWatcher.setFuture(QtConcurrent::run([](QString path, Target target, Format format)->QSharedPointer<Bitmap> {
            QSharedPointer<Bitmap> bitmap(new Bitmap);
            bitmap->target = target;

            QFile content;
            if (path.startsWith("qrc:", Qt::CaseInsensitive))
            {
                content.setFileName(path.mid(3)); // Keep the ':'. This is not a mistake.
            }
            else if (path.startsWith("file:", Qt::CaseInsensitive))
            {
                content.setFileName(QUrl(path).toLocalFile());
            }
            else
            {
                content.setFileName(path);
            }

            if (content.open(QFile::ReadOnly))
            {
                QByteArray encodedData = content.readAll();
                bitmap->decodedData = stbi_load_from_memory((const uint8_t *)encodedData.constData(), encodedData.size(), &bitmap->width, &bitmap->height, &bitmap->channelCount, format);

                if (!bitmap->decodedData)
                {
                    qCritical() << "Could not decode" << path << ":" << stbi_failure_reason();
                }

                content.close();
            }
            else
            {
                qCritical() << path << "not found!";
            }

            return bitmap;
        }, m_source, (Target)m_target, m_format));
    }
}

void Texture::onImageDecodingComplete()
{
    QSharedPointer<Bitmap> bitmap = m_dataFutureWatcher.result();
    if (bitmap->decodedData)
    {
        setStatus(StatusProcessing);
    }
    else
    {
        setStatus(StatusError);
    }
}

void Texture::onUpdate()
{
    switch(status())
    {
    case StatusProcessing:
    {
        QSharedPointer<Bitmap> bitmap = m_dataFutureWatcher.result();

        GLenum glformat, gltype = GL_UNSIGNED_BYTE;
        switch(bitmap->channelCount)
        {
        case 1:
            glformat = GL_RED;
            break;
        case 2:
            glformat = GL_RG;
            break;
        case 3:
            glformat = GL_RGB;
            break;
        case 4:
            glformat = GL_RGBA;
            break;
        default:
            qCritical() << "Unknown texture format:" << bitmap->channelCount;
            setStatus(StatusError);
            return;
        }

        GLenum glTarget;
        switch (bitmap->target)
        {
        case TargetTexture2D:
            glTarget = GL_TEXTURE_2D;
            device()->glEnable(GL_TEXTURE_2D);
            break;
        case TargetTextureCubeMap:
            glTarget = GL_TEXTURE_CUBE_MAP;
            device()->glEnable(GL_TEXTURE_CUBE_MAP);
            break;
        default:
            qCritical() << "Unknown texture target:" << bitmap->target;
            setStatus(StatusError);
            return;
        }

        if (!handle())
        {
            device()->glGenTextures(1, &m_handle);
        }

        device()->glBindTexture(glTarget, handle());
        device()->glTexImage2D(glTarget, 0, glformat, bitmap->width, bitmap->height, 0, glformat, gltype, bitmap->decodedData);
        device()->glGenerateMipmap(glTarget);

        setSampleMode(Sample_Anisotropic | Sample_Clamp);

        stbi_image_free(bitmap->decodedData);
        bitmap->decodedData = nullptr;
        break;
    }
    default:
        break;
    }
}

void Texture::onShutdown()
{
    if (handle())
    {
        device()->glDeleteTextures(1, &m_handle);
        m_handle = 0;
    }
}


