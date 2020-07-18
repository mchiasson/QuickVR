#include "Node.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include "Device.h"

Device* Node::m_device = nullptr;

Node::Node(Node *parent) : QQuickItem(parent)
{
    setObjectName("Node");
}

void Node::setAngularVelocity(const QVector3D &newAngularVelocity)
{
    if (m_angularVelocity != newAngularVelocity)
    {
        m_angularVelocity = newAngularVelocity;
        emit angularVelocityChanged(newAngularVelocity);
    }
}

void Node::setLinearVelocity(const QVector3D &newLinearVelocity)
{
    if (m_linearVelocity != newLinearVelocity)
    {
        m_linearVelocity = newLinearVelocity;
        emit linearVelocityChanged(newLinearVelocity);
    }
}

void Node::componentComplete()
{
    Node* pNode = this;

    while(!m_device && pNode) {
        pNode = qobject_cast<Node*>(pNode->parentItem());
        if (pNode) m_device = pNode->m_device;
    }

    if(!m_device)
    {
        qCritical("%s must be located under a Device tree", qPrintable(objectName()));
        qmlEngine(this)->exit(-1);
        return;
    }

    connect(m_device, &Device::init,  [&]() {
        onInit();
    });

    connect(m_device, &Device::shutdown,  [&]() {
        onShutdown();
    });

    connect(m_device, &Device::update, [&]() {

        // TODO rotate on all three axis.

        float rotY = rotation() + (m_angularVelocity.y() * device()->dt());

        QVector3D offset = QQuaternion::fromAxisAndAngle(0, 1, 0, rotY) * (m_linearVelocity * device()->dt());

        setX(x() + offset.x());
        setY(y() + offset.y());
        setZ(z() + offset.z());

        setRotation(rotY);
        onUpdate();
    });

    QQuickItem::componentComplete();
}

