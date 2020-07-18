#include "SpotLight.h"

void SpotLight::setDirection(const QVector3D &newDirection)
{
    if (m_direction != newDirection)
    {
        m_direction = newDirection;
        emit directionChanged(newDirection);
    }
}

void SpotLight::setCutoff(qreal newCutoff)
{
    if (m_cutoff != newCutoff)
    {
        m_cutoff = newCutoff;
        emit cutoffChanged(newCutoff);
    }
}
