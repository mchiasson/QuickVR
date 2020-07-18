#include "Texture.h"

#include <stb_image.h>

Texture::~Texture()
{
    if (m_texId)
    {
        m_device->glDeleteTextures(1, &m_texId);
        m_texId = 0;
    }
}

bool Texture::load(const QString &path, int format, int sampleMode)
{
    int width, height, comp;
    QByteArray contentData;
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
        contentData = content.readAll();
        content.close();
    }
    else
    {
        qCritical() << path << "not found!";
        return false;
    }

    unsigned char *data = stbi_load_from_memory((const uint8_t *)contentData.constData(), contentData.size(), &width, &height, &comp, STBI_rgb_alpha);

    if (nullptr != data)
    {
        if (m_texId == 0)
        {
            m_device->glGenTextures(1, &m_texId);
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

        m_textureTarget = GL_TEXTURE_2D;

        if (format & Texture_Cubemap)
        {
            m_textureTarget = GL_TEXTURE_CUBE_MAP;
            m_device->glEnable(GL_TEXTURE_CUBE_MAP);
        }

        m_device->glBindTexture(m_textureTarget, m_texId);
        m_device->glTexImage2D(m_textureTarget, 0, internalFormat, width, height, 0, glformat, gltype, data);
        m_device->glGenerateMipmap(m_textureTarget);

        setSampleMode(sampleMode);

        stbi_image_free(data);
        return true;
    }
    else
    {
        qCritical() << "Could not load" << path << ":" << stbi_failure_reason();
    }

    return false;
}

void Texture::bind(int slot) const
{
    if (m_texId == 0) return;
    m_device->glActiveTexture(GL_TEXTURE0 + slot);
    m_device->glBindTexture(m_textureTarget, m_texId);
}

void Texture::setSampleMode(int sm)
{
    if (m_texId == 0) return;

    bind(0);

    switch (sm & Sample_FilterMask)
    {
    case Sample_Linear:
        m_device->glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        m_device->glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        if(GL_EXT_texture_filter_anisotropic)
            m_device->glTexParameteri(m_textureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
        break;

    case Sample_Anisotropic:
        m_device->glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        m_device->glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        if(GL_EXT_texture_filter_anisotropic)
            m_device->glTexParameteri(m_textureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);
        break;

    case Sample_Nearest:
        m_device->glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        m_device->glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        if(GL_EXT_texture_filter_anisotropic)
            m_device->glTexParameteri(m_textureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
        break;
    }

    switch (sm & Sample_AddressMask)
    {
    case Sample_Repeat:
        m_device->glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
        m_device->glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);
        break;

    case Sample_Clamp:
        m_device->glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        m_device->glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        break;

    case Sample_ClampBorder:
        m_device->glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        m_device->glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        break;
    }
}



