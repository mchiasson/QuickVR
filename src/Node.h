#ifndef NODE_H
#define NODE_H

#include <QQuickItem>
#include <QVector3D>

#define direct_connect(sender, senderSignal, receiver, receiverSlot) connect(sender, senderSignal, receiver, receiverSlot, Qt::ConnectionType(Qt::DirectConnection|Qt::UniqueConnection))

class Device;

class Node : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QVector3D angularVelocity READ angularVelocity WRITE setAngularVelocity NOTIFY angularVelocityChanged)
    Q_PROPERTY(QVector3D linearVelocity READ linearVelocity WRITE setLinearVelocity NOTIFY linearVelocityChanged)

    friend class Device;

public:
    explicit Node(Node *parent = nullptr);

    const QVector3D &angularVelocity() const { return m_angularVelocity; }
    const QVector3D &linearVelocity() const { return m_linearVelocity; }

    void setAngularVelocity(const QVector3D &newAngularVelocity);
    void setLinearVelocity(const QVector3D &newLinearVelocity);

    void setDevice(Device* newDevice);

signals:

    void angularVelocityChanged(const QVector3D &newAngularVelocity);
    void linearVelocityChanged(const QVector3D &newLinearVelocity);

protected slots:

    void onUpdateGUIThread();

protected:

    static Device *device() { return s_device; }

    virtual void onInit() {}
    virtual void onShutdown() {}
    virtual void onUpdate() {};

    void componentComplete() override;

    QVector3D m_angularVelocity;
    QVector3D m_linearVelocity;

private:

    static Device* s_device;
};

#endif // NODE_H
