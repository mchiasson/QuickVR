#include "QuickVR_plugin.h"

#include "Device.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "ShaderProgram.h"
#include "SkinnedMesh.h"
#include "SpotLight.h"
#include "Headset.h"

#include <qqml.h>

void QuickVRPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<Device>           (uri, 1, 0, "Device");
    qmlRegisterType<DirectionalLight> (uri, 1, 0, "DirectionalLight");
    qmlRegisterType<Node>             (uri, 1, 0, "Node");
    qmlRegisterType<PointLight>       (uri, 1, 0, "PointLight");
    qmlRegisterType<Shader>           (uri, 1, 0, "Shader");
    qmlRegisterType<ShaderProgram>    (uri, 1, 0, "ShaderProgram");
    qmlRegisterType<SkinnedMesh>      (uri, 1, 0, "SkinnedMesh");
    qmlRegisterType<SpotLight>        (uri, 1, 0, "SpotLight");
    qmlRegisterType<Headset>          (uri, 1, 0, "Headset");

    // unexposed to the users in QML but must be visible to the QML engine.
    qmlRegisterUncreatableType<BaseLight>(uri, 1, 0, "BaseLight", QStringLiteral("Base type for DirectionalLight, PointLight and SpotLight"));

}

