#ifndef SPOTLIGHT_H
#define SPOTLIGHT_H

#include "PointLight.h"

class SpotLight : public PointLight
{
    Q_OBJECT

    Q_PROPERTY(QVector3D direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(qreal     cutoff    READ cutoff    WRITE setCutoff    NOTIFY cutoffChanged)

public:
    explicit SpotLight(Node *parent = nullptr) : PointLight(parent) {}

    const QVector3D &direction() const { return m_direction; }
    qreal cutoff() const { return m_cutoff; }

    void setDirection(const QVector3D &newDirection);
    void setCutoff(qreal newCutoff);

signals:

    void directionChanged(const QVector3D &newDirection);
    void cutoffChanged(qreal newCutoff);

protected:

    QVector3D m_direction;
    qreal m_cutoff = 0.0;

};

#endif // SPOTLIGHT_H
