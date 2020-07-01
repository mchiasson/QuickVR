#ifndef VRHEADSET_H
#define VRHEADSET_H

#include <QQuickItem>
#include <QVector3D>
#include <QQuaternion>

class VRWindow;

class VRHeadset : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QVector3D angularVelocity READ angularVelocity WRITE setAngularVelocity NOTIFY angularVelocityChanged)
    Q_PROPERTY(QVector3D linearVelocity READ linearVelocity WRITE setLinearVelocity NOTIFY linearVelocityChanged)

public:
    explicit VRHeadset(QQuickItem *parent = nullptr);

    const QVector3D &angularVelocity() const { return m_angularVelocity; }
    const QVector3D &linearVelocity() const { return m_linearVelocity; }

    void setAngularVelocity(const QVector3D &newAngularVelocity);
    void setLinearVelocity(const QVector3D &newLinearVelocity);

signals:

    void angularVelocityChanged(const QVector3D &);
    void linearVelocityChanged(const QVector3D &);

private slots:
    void sync();
    void handleWindowChanged(QQuickWindow *win);
    void onXChanged();
    void onYChanged();
    void onZChanged();
    void onRotationChanged();
private:

    VRWindow *m_vrWindow = nullptr;
    QVector3D m_angularVelocity;
    QVector3D m_linearVelocity;
};

#endif // VRHEADSET_H
