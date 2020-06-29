#ifndef VRWINDOW_H
#define VRWINDOW_H

#include <QQuickView>
#include <QVector3D>

class VRRenderer;

class VRWindow : public QQuickView
{
    Q_OBJECT
    Q_DISABLE_COPY(VRWindow)

public:
    explicit VRWindow(QWindow *parent = nullptr);
    ~VRWindow() override;

public slots:
    void sync();

private:
    VRRenderer * m_renderer = nullptr;
};

#endif // VRWINDOW_H
