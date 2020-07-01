#include "VRHeadset.h"

#include <QtMath>

#include "VRRenderer.h"
#include "VRWindow.h"

VRHeadset::VRHeadset(QQuickItem *parent)
    : QQuickItem(parent)
{
    connect(this, &QQuickItem::windowChanged, this, &VRHeadset::handleWindowChanged, Qt::DirectConnection);
    connect(this, &QQuickItem::xChanged, this, &VRHeadset::onXChanged, Qt::DirectConnection);
    connect(this, &QQuickItem::yChanged, this, &VRHeadset::onYChanged, Qt::DirectConnection);
    connect(this, &QQuickItem::zChanged, this, &VRHeadset::onZChanged, Qt::DirectConnection);
    connect(this, &QQuickItem::rotationChanged, this, &VRHeadset::onRotationChanged, Qt::DirectConnection);
}

void VRHeadset::setAngularVelocity(const QVector3D &newAngularVelocity)
{
    if (m_angularVelocity != newAngularVelocity)
    {
        m_angularVelocity = newAngularVelocity;
        emit angularVelocityChanged(newAngularVelocity);
    }
}

void VRHeadset::setLinearVelocity(const QVector3D &newLinearVelocity)
{
    if (m_linearVelocity != newLinearVelocity)
    {
        m_linearVelocity = newLinearVelocity;
        emit linearVelocityChanged(newLinearVelocity);
    }
}

void VRHeadset::sync()
{
    QVector3D offset = QQuaternion::fromAxisAndAngle(0, 1, 0, rotation() + m_angularVelocity.y()) * m_linearVelocity;

    setX(x() + offset.x());
    setY(y() + offset.y());
    setZ(z() + offset.z());

    setRotation(rotation() + m_angularVelocity.y());
}

void VRHeadset::handleWindowChanged(QQuickWindow *win)
{
    if (m_vrWindow)
    {
        disconnect(m_vrWindow, &VRWindow::beforeRendering, this, &VRHeadset::sync);
    }

    m_vrWindow = qobject_cast<VRWindow *>(win);

    if (m_vrWindow)
    {
        connect(m_vrWindow, &VRWindow::beforeRendering, this, &VRHeadset::sync, Qt::DirectConnection);
        m_vrWindow->renderer()->Orientation = QQuaternion::fromAxisAndAngle(0, 1, 0, rotation());
        m_vrWindow->renderer()->Position = QVector3D(x(), y(), z());
    }
}

void VRHeadset::onXChanged()
{
    if (m_vrWindow)
    {
        m_vrWindow->renderer()->Position.setX(x());
    }
}

void VRHeadset::onYChanged()
{
    if (m_vrWindow)
    {
        m_vrWindow->renderer()->Position.setY(y());
    }
}

void VRHeadset::onZChanged()
{
    if (m_vrWindow)
    {
        m_vrWindow->renderer()->Position.setZ(z());
    }
}

void VRHeadset::onRotationChanged()
{
    if (m_vrWindow)
    {
        m_vrWindow->renderer()->Orientation = QQuaternion::fromAxisAndAngle(0, 1, 0, rotation());
    }
}
