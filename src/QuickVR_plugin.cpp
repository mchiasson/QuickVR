#include "QuickVR_plugin.h"

#include "VRHeadset.h"
#include "VRWindow.h"

#include <qqml.h>

void QuickVRPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<VRHeadset>(uri, 1, 0, "VRHeadset");
    qmlRegisterType<VRWindow>(uri, 1, 0, "VRWindow");
}

