#include "VRWindow.h"
#include "VRRenderer.h"
#include "Node.h"

VRWindow::VRWindow(QWindow *parent)
    : QQuickView(parent)
{
    direct_connect(this, &QQuickWindow::sceneGraphInitialized, this, &VRWindow::initRenderer);
}

VRWindow::~VRWindow()
{
    if (m_renderer)
    {
        delete m_renderer;
        m_renderer = nullptr;
    }
}

void VRWindow::initRenderer()
{
    if (!m_renderer) {
        m_renderer = new VRRenderer(this);
        direct_connect(this, &QQuickWindow::beforeRendering,           m_renderer, &VRRenderer::init);
        direct_connect(this, &QQuickWindow::beforeRenderPassRecording, m_renderer, &VRRenderer::paint);
        direct_connect(this, &QQuickWindow::sceneGraphInvalidated,     m_renderer, &VRRenderer::cleanup);
    }
}
