#ifndef BASELIGHT_H
#define BASELIGHT_H

#include "Node.h"
#include <QVector3D>

class BaseLight : public Node
{
    Q_OBJECT
    Q_PROPERTY(QColor color            READ color            WRITE setColor            NOTIFY colorChanged)
    Q_PROPERTY(qreal  ambientIntensity READ ambientIntensity WRITE setAmbientIntensity NOTIFY ambientIntensityChanged)
    Q_PROPERTY(qreal  diffuseIntensity READ diffuseIntensity WRITE setDiffuseIntensity NOTIFY diffuseIntensityChanged)

public:

    explicit BaseLight(Node *parent = nullptr) : Node(parent) {}

    const QColor &color() const { return m_color; }
    qreal ambientIntensity() const { return m_ambientIntensity; }
    qreal diffuseIntensity() const { return m_diffuseIntensity; }

    void setColor(const QColor &newColor);
    void setAmbientIntensity(qreal newAmbientIntensity);
    void setDiffuseIntensity(qreal newDiffuseIntensity);

signals:

    void colorChanged(const QColor &newColor);
    void ambientIntensityChanged(qreal newAmbientIntensity);
    void diffuseIntensityChanged(qreal newDiffuseIntensity);

protected:

    QColor m_color;
    qreal m_ambientIntensity = 0.0;
    qreal m_diffuseIntensity = 0.0;

};
#endif // BASELIGHT_H
