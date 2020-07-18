#ifndef POINTLIGHT_H
#define POINTLIGHT_H

#include "BaseLight.h"

class PointLight : public BaseLight
{
    Q_OBJECT

    Q_PROPERTY(qreal attenuationConstant READ attenuationConstant WRITE setAttenuationConstant NOTIFY attenuationConstantChanged)
    Q_PROPERTY(qreal attenuationLinear   READ attenuationLinear   WRITE setAttenuationLinear   NOTIFY attenuationLinearChanged)
    Q_PROPERTY(qreal attenuationExp      READ attenuationExp      WRITE setAttenuationExp      NOTIFY attenuationExpChanged)

public:
    explicit PointLight(Node *parent = nullptr) : BaseLight(parent) {}

    qreal attenuationConstant() const { return m_attenuationConstant; }
    qreal attenuationLinear() const { return m_attenuationLinear; }
    qreal attenuationExp() const { return m_attenuationExp; }

    void setAttenuationConstant(qreal newAttenuationConstant);
    void setAttenuationLinear(qreal newAttenuationLinear);
    void setAttenuationExp(qreal newAttenuationExp);

signals:

    void attenuationConstantChanged(qreal newAttenuationConstant);
    void attenuationLinearChanged(qreal newAttenuationLinear);
    void attenuationExpChanged(qreal newAttenuationExp);

protected:

    qreal m_attenuationConstant = 1.0;
    qreal m_attenuationLinear = 0.0;
    qreal m_attenuationExp = 0.0;
};

#endif // POINTLIGHT_H
