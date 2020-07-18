#ifndef DIRECTIONALLIGHT_H
#define DIRECTIONALLIGHT_H

#include "BaseLight.h"

class DirectionalLight : public BaseLight
{
    Q_OBJECT
    Q_PROPERTY(QVector3D direction READ direction WRITE setDirection NOTIFY directionChanged)
public:
    explicit DirectionalLight(Node * parent = nullptr) : BaseLight(parent) {}

    const QVector3D &direction() const { return m_direction; }

    void setDirection(const QVector3D &newDirection);

signals:

    void directionChanged(const QVector3D &newDirection);

protected:

    QVector3D m_direction;
};

#endif // DIRECTIONALLIGHT_H
