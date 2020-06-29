#include "QuickVR_plugin.h"

#include "VRWindow.h"

#include <qqml.h>

void QuickVRPlugin::registerTypes(const char *uri)
{
    // @uri QuickVR
    qmlRegisterType<VRWindow>(uri, 1, 0, "VRWindow");
}

