#include "Node.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include "Device.h"

Device* Node::s_device = nullptr;

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

void Node::setDevice(Device* newDevice)
{
    connect(newDevice, &Device::init,  [&]() {
        onInit();
    });

    connect(newDevice, &Device::shutdown,  [&]() {
        onShutdown();
    });

    connect(newDevice, &Device::update, [&]() {
        onUpdate();
    });
}

void Node::onUpdateGUIThread()
{
    // TODO rotate on all three axis.

    float rotY = rotation() + (m_angularVelocity.y() * device()->dt());

    QVector3D offset = QQuaternion::fromAxisAndAngle(0, 1, 0, rotY) * (m_linearVelocity * device()->dt());

    setX(x() + offset.x());
    setY(y() + offset.y());
    setZ(z() + offset.z());

    setRotation(rotY);
}

void Node::componentComplete()
{
    Node* pNode = this;

    while(!s_device && pNode) {
        pNode = qobject_cast<Node*>(pNode->parentItem());
        if (pNode) s_device = pNode->s_device;
    }

    if(!s_device)
    {
        qCritical("%s must be located under a Device tree", qPrintable(objectName()));
        qmlEngine(this)->exit(-1);
        return;
    }

    setDevice(s_device);
    connect(s_device, &Device::update, this, &Node::onUpdateGUIThread);

    QQuickItem::componentComplete();
}

