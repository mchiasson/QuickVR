#ifndef TEXTURE_H
#define TEXTURE_H

#include <OVR_CAPI_GL.h>

#include "Device.h"

class Texture : Node
{
public:
    virtual ~Texture() override;

    void setDevice(Device* device) {
        m_device = device;
    }

    GLuint getTexId() const { return m_texId; }

    bool load(const QString &path, int format = Texture_RGBA, int sampleMode = Sample_Anisotropic | Sample_Clamp);
    void bind(int slot) const;
    void setSampleMode(int);

protected:
    Device* m_device = nullptr;
    GLenum  m_textureTarget = 0;
    GLuint  m_texId = 0;
};

#endif // TEXTURE_H
