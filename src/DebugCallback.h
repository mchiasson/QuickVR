#ifndef DEBUGCALLBACK_H
#define DEBUGCALLBACK_H

#include <QtGui/QOpenGLExtraFunctions>

class VRRenderer;

namespace GL {
    namespace DebugCallback {
        void install(QOpenGLExtraFunctions *api);
    };
} // namespace GL

#endif // DEBUGCALLBACK_H
