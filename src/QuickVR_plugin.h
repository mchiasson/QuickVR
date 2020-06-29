#ifndef QUICKVR_PLUGIN_H
#define QUICKVR_PLUGIN_H

#include <QQmlExtensionPlugin>

class QuickVRPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override;
};

#endif // QUICKVR_PLUGIN_H
