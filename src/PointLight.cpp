#include "PointLight.h"

void PointLight::setAttenuationConstant(qreal newAttenuationConstant)
{
    if (m_attenuationConstant != newAttenuationConstant)
    {
        m_attenuationConstant = newAttenuationConstant;
        emit attenuationConstantChanged(newAttenuationConstant);
    }
}

void PointLight::setAttenuationLinear(qreal newAttenuationLinear)
{
    if (m_attenuationLinear != newAttenuationLinear)
    {
        m_attenuationLinear = newAttenuationLinear;
        emit attenuationLinearChanged(newAttenuationLinear);
    }
}

void PointLight::setAttenuationExp(qreal newAttenuationExp)
{
    if (m_attenuationExp != newAttenuationExp)
    {
        m_attenuationExp = newAttenuationExp;
        emit attenuationExpChanged(newAttenuationExp);
    }
}
