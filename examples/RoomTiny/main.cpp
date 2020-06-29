#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // This line is only necessary for QuickVR examples. Once QuickVR is
    // deployed/installed on your system, the application will have no trouble
    // finding quickvrplugin.dll or libquickvrplugin.so
    engine.addImportPath(app.applicationDirPath() + "/../../../src/imports");

    engine.load("qrc:/main.qml");

    return app.exec();
}
