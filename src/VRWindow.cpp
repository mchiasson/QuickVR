#include "VRWindow.h"
#include "VRRenderer.h"

VRWindow::VRWindow(QWindow *parent)
    : QQuickView(parent)
{
    connect(this, &QQuickWindow::beforeSynchronizing, this, &VRWindow::sync, Qt::DirectConnection);

    QSurfaceFormat format = requestedFormat();
    // Turn off vsync to let the compositor do its magic
    format.setSwapInterval(0);
    setFormat(format);
}

VRWindow::~VRWindow()
{
    if (m_renderer)
    {
        delete m_renderer;
        m_renderer = nullptr;
    }
}

void VRWindow::sync()
{
    if (!m_renderer) {
        m_renderer = new VRRenderer(this);
        connect(this, &QQuickWindow::beforeRendering,           m_renderer, &VRRenderer::init,    Qt::DirectConnection);
        connect(this, &QQuickWindow::beforeRenderPassRecording, m_renderer, &VRRenderer::paint,   Qt::DirectConnection);
        connect(this, &QQuickWindow::sceneGraphInvalidated,     m_renderer, &VRRenderer::cleanup, Qt::DirectConnection);
    }
    m_renderer->setViewportSize(size() * devicePixelRatio());
}
