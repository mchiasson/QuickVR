#include "DirectionalLight.h"

void DirectionalLight::setDirection(const QVector3D &newDirection)
{
    if (m_direction != newDirection)
    {
        m_direction = newDirection;
        emit directionChanged(newDirection);
    }
}
