#include "BaseLight.h"

void BaseLight::setColor(const QColor &newColor)
{
    if (m_color != newColor)
    {
        m_color = newColor;
        emit colorChanged(newColor);
    }
}

void BaseLight::setAmbientIntensity(qreal newAmbientIntensity)
{
    if (m_ambientIntensity != newAmbientIntensity)
    {
        m_ambientIntensity = newAmbientIntensity;
        emit ambientIntensityChanged(newAmbientIntensity);
    }
}

void BaseLight::setDiffuseIntensity(qreal newDiffuseIntensity)
{
    if (m_diffuseIntensity != newDiffuseIntensity)
    {
        m_diffuseIntensity = newDiffuseIntensity;
        emit diffuseIntensityChanged(newDiffuseIntensity);
    }
}
