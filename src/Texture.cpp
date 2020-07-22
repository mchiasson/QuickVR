#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <QDebug>
#include <QMutexLocker>

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
    connect(&m_dataFutureWatcher, &QFutureWatcher<QByteArray>::finished, this, &Texture::onImageDecodingComplete);
}

void Texture::setSource(const QString &newSource)
{
    QMutexLocker lock(&m_mutex);
    if (m_source != newSource)
    {
        m_source = newSource;
        emit sourceChanged(newSource);
        setStatus(StatusLoading);
        m_dirtyFlag |= DirtySource;
    }
}

void Texture::setTarget(Target newTarget)
{
    QMutexLocker lock(&m_mutex);
    if (m_target != newTarget)
    {
        m_target = newTarget;
        emit targetChanged(newTarget);
        setStatus(StatusLoading);
        m_dirtyFlag |= DirtyTarget;
    }
}

void Texture::setFormat(Format newFormat)
{
    QMutexLocker lock(&m_mutex);
    if (m_format != newFormat)
    {
        m_format = newFormat;
        emit formatChanged(newFormat);
        setStatus(StatusLoading);
        m_dirtyFlag |= DirtyFormat;
    }
}

void Texture::setFilter(Filter newFilter)
{
    QMutexLocker lock(&m_mutex);
    if (m_filter != newFilter)
    {
        m_filter = newFilter;
        emit filterChanged(newFilter);
        m_dirtyFlag |= DirtyFilter;
    }
}

void Texture::setWrapS(Wrap newWrap)
{
    QMutexLocker lock(&m_mutex);
    if (m_wrapS != newWrap)
    {
        m_wrapS = newWrap;
        emit wrapSChanged(newWrap);
        m_dirtyFlag |= DirtyWrapS;
    }
}

void Texture::setWrapT(Wrap newWrap)
{
    QMutexLocker lock(&m_mutex);
    if (m_wrapT != newWrap)
    {
        m_wrapT = newWrap;
        emit wrapTChanged(newWrap);
        m_dirtyFlag |= DirtyWrapT;
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


void Texture::bind(int slot)
{
    if (handle())
    {
        device()->glActiveTexture(GL_TEXTURE0 + slot);
        device()->glBindTexture(target(), handle());
    }
}

void Texture::onImageDecodingComplete()
{
    QSharedPointer<Bitmap> bitmap = m_dataFutureWatcher.result();
    if (bitmap->decodedData)
    {
        setStatus(StatusProcessing);
        m_dirtyFlag |= DirtyBitmap;
    }
    else
    {
        setStatus(StatusError);
    }
}

void Texture::onUpdate()
{
    if (m_dirtyFlag == DirtyNone)
        return;

    QMutexLocker lock(&m_mutex);

    if (!handle())
    {
        device()->glGenTextures(1, &m_handle);
    }

    if (m_dirtyFlag & (DirtySource|DirtyTarget|DirtyFormat))
    {
        // Load the scene from a thread in Qt's thread pool.
        m_dataFutureWatcher.setFuture(QtConcurrent::run([](QString path, Target target, Format format)->QSharedPointer<Bitmap> {
            QSharedPointer<Bitmap> bitmap(new Bitmap);

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
                int requestDecodeFormat = 0;
                switch (format)
                {
                case FormatGrey:      requestDecodeFormat = STBI_grey;       break;
                case FormatGreyAlpha: requestDecodeFormat = STBI_grey_alpha; break;
                case FormatRGB:       requestDecodeFormat = STBI_rgb;        break;
                case FormatRGBA:      requestDecodeFormat = STBI_rgb_alpha;  break;
                }

                bitmap->decodedData = stbi_load_from_memory((const uint8_t *)encodedData.constData(), encodedData.size(), &bitmap->width, &bitmap->height, &bitmap->channelCount, requestDecodeFormat);

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
        }, source(), target(), format()));

        m_dirtyFlag &= ~(DirtySource|DirtyTarget|DirtyFormat);
    }

    if (m_dirtyFlag & DirtyBitmap)
    {
        QSharedPointer<Bitmap> bitmap = m_dataFutureWatcher.result();

        switch (target())
        {
        case GL_TEXTURE_2D:
            device()->glEnable(GL_TEXTURE_2D);
            break;
        case GL_TEXTURE_CUBE_MAP:
            device()->glEnable(GL_TEXTURE_CUBE_MAP);
            break;
        default:
            qCritical() << "Unknown texture target:" << target();
            setStatus(StatusError);
            return;
        }

        switch(format())
        {
        case FormatGrey:
            device()->glPixelStorei(GL_PACK_ALIGNMENT, 1);
            device()->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            break;
        case FormatGreyAlpha:
            device()->glPixelStorei(GL_PACK_ALIGNMENT, 2);
            device()->glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
            break;
        case FormatRGB:
            device()->glPixelStorei(GL_PACK_ALIGNMENT, 1);
            device()->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            break;
        case FormatRGBA:
            device()->glPixelStorei(GL_PACK_ALIGNMENT, 4);
            device()->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            break;
        }

        device()->glBindTexture(target(), handle());
        device()->glTexImage2D(target(), 0, format(), bitmap->width, bitmap->height, 0, format(), GL_UNSIGNED_BYTE, bitmap->decodedData);
        device()->glGenerateMipmap(target());

        stbi_image_free(bitmap->decodedData);
        bitmap->decodedData = nullptr;

        m_dirtyFlag &= ~(DirtyBitmap);
    }

    if (m_dirtyFlag & DirtyFilter)
    {
        device()->glBindTexture(target(), handle());

        switch(filter())
        {
        case FilterNone:
            device()->glTexParameteri(target(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            device()->glTexParameteri(target(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            device()->glTexParameteri(target(), GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
            break;
        case FilterBilinear:
            device()->glTexParameteri(target(), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            device()->glTexParameteri(target(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            device()->glTexParameteri(target(), GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
            break;
        case FilterTrilinear:
            device()->glTexParameteri(target(), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            device()->glTexParameteri(target(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            device()->glTexParameteri(target(), GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
            break;
        case FilterAnisotropic:
            device()->glTexParameteri(target(), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            device()->glTexParameteri(target(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            device()->glTexParameteri(target(), GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);
            break;
        }
        m_dirtyFlag &= ~(DirtyFilter);
    }

    if (m_dirtyFlag & DirtyWrapS)
    {
        device()->glBindTexture(target(), handle());
        device()->glTexParameteri(target(), GL_TEXTURE_WRAP_S, wrapS());
        m_dirtyFlag &= ~(DirtyWrapS);
    }

    if (m_dirtyFlag & DirtyWrapT)
    {
        device()->glBindTexture(target(), handle());
        device()->glTexParameteri(target(), GL_TEXTURE_WRAP_T, wrapT());
        m_dirtyFlag &= ~(DirtyWrapT);
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


