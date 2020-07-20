#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::Bitmap::~Bitmap()
{
    if (data)
    {
        stbi_image_free(data);
        data = nullptr;
    }
}

Texture::Texture(Node *parent) : Node(parent)
{
    connect(&m_dataFutureWatcher, &QFutureWatcher<QByteArray>::finished, this, &Texture::onImageDecodingComplete);
}

void Texture::setSource(QString newSource)
{
    if (m_source != newSource)
    {
        m_source = newSource;
        emit sourceChanged(newSource);

        setStatus(Loading);

        // Load the scene from a thread in Qt's thread pool.
        m_dataFutureWatcher.setFuture(QtConcurrent::run([](QString path)->QSharedPointer<Bitmap> {
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
                bitmap->data = stbi_load_from_memory((const uint8_t *)encodedData.constData(), encodedData.size(), &bitmap->width, &bitmap->height, &bitmap->channelCount, STBI_rgb_alpha);

                if (!bitmap->data)
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
        }, newSource));

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

void Texture::onImageDecodingComplete()
{
    QSharedPointer<Bitmap> bitmap = m_dataFutureWatcher.result();
    if (bitmap->data)
    {
        setStatus(Processing);
    }
    else
    {
        setStatus(Error);
    }
}

void Texture::onUpdate()
{
    switch(status())
    {
    case Processing:
    {
        QSharedPointer<Bitmap> bitmap = m_dataFutureWatcher.result();

        //TODO
        int format = Texture_RGBA;
        int sampleMode = Sample_Anisotropic | Sample_Clamp;

        if (handle() == 0)
        {
            device()->glGenTextures(1, &m_handle);
        }

        bool isDepth = ((format & Texture_DepthMask) != 0);
        bool isSRGB  = ((format & Texture_SRGB) != 0);
        Q_ASSERT(!(isSRGB && isDepth) && "Depth formats and sRGB are incompatible texture flags.");

        bool isFloatFormat = (((format & Texture_TypeMask) == Texture_RGBA16f) ||
                              ((format & Texture_TypeMask) == Texture_R11G11B10f));

        bool isCompressed = false;

        // if this is a color texture that will be sent over to or received from OVRService, then force sRGB on it
        // we will keep rendering into it as if it's a linear-format texture
        if (!isDepth && !isFloatFormat &&
            ((format & Texture_SwapTextureSet) || (format & Texture_SwapTextureSetStatic) || (format & Texture_Mirror)))
        {
            isSRGB = true;
        }

        GLenum glformat = GL_RGBA, gltype = GL_UNSIGNED_BYTE;
        GLenum internalFormat = isSRGB ? GL_SRGB8_ALPHA8 : glformat;
        switch(format & Texture_TypeMask)
        {
        case Texture_B5G6R5:
            glformat = GL_RGB565;
            break;
        case Texture_BGR5A1:
            glformat = GL_RGB5_A1;
            break;
        case Texture_BGRA4:
            glformat = GL_RGBA4;
            break;
        case Texture_RGBA8:
            glformat = GL_RGBA;
            break;
        case Texture_BGRA8:
            glformat = GL_BGRA;
            break;
        case Texture_BGRX:
            glformat = GL_BGR;
            break;
        case Texture_RGBA16f:
            glformat = GL_RGBA16F;
            // float formats use special higher-precision internal formats and don't do sRGB
            internalFormat = glformat;
            break;
        case Texture_R11G11B10f:
            glformat = GL_R11F_G11F_B10F;
            // float formats use special higher-precision internal formats and don't do sRGB
            internalFormat = glformat;
            break;
        case Texture_R:
            glformat = GL_RED;
            break;
        case Texture_A:
            glformat = GL_ALPHA;
            break;
        case Texture_Depth32f:
            glformat = GL_DEPTH_COMPONENT;
            internalFormat = GL_ARB_depth_buffer_float ? GL_DEPTH_COMPONENT32F : GL_DEPTH_COMPONENT;
            gltype = GL_FLOAT;
            break;
        case Texture_Depth16:
            glformat = GL_DEPTH_COMPONENT;
            internalFormat = GL_DEPTH_COMPONENT16;
            gltype = GL_UNSIGNED_INT;
            break;
        case Texture_Depth24Stencil8:
            glformat = GL_DEPTH_STENCIL;
            internalFormat = GL_DEPTH24_STENCIL8;
            gltype = GL_UNSIGNED_INT_24_8;
            break;
        case Texture_Depth32fStencil8:
            glformat = GL_DEPTH_STENCIL;
            internalFormat = GL_DEPTH32F_STENCIL8;
            gltype = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
            break;
        case Texture_BC1:
            glformat = isSRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            isCompressed = true;
            break;
        case Texture_BC2:
            glformat = isSRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            isCompressed = true;
            break;
        case Texture_BC3:
            glformat = isSRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            isCompressed = true;
            break;
        case Texture_BC6U:
            glformat = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB;
            isCompressed = true;
            break;
        case Texture_BC6S:
            glformat = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB;
            isCompressed = true;
            break;
        case Texture_BC7:
            glformat = isSRGB ? GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB : GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
            isCompressed = true;
            break;
        default:
            qFatal("Unknown texture format");
        }

        m_target = GL_TEXTURE_2D;

        if (format & Texture_Cubemap)
        {
            m_target = GL_TEXTURE_CUBE_MAP;
            device()->glEnable(GL_TEXTURE_CUBE_MAP);
        }

        device()->glBindTexture(target(), handle());
        device()->glTexImage2D(target(), 0, internalFormat, bitmap->width, bitmap->height, 0, glformat, gltype, bitmap->data);
        device()->glGenerateMipmap(target());

        setSampleMode(sampleMode);

        stbi_image_free(bitmap->data);
        bitmap->data = nullptr;
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


