#include <QGuiApplication>
#include <QQmlApplicationEngine>
//#include <QtGui/QSurfaceFormat>

int main(int argc, char *argv[])
{
//    QSurfaceFormat ftm;
//    ftm.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
//    ftm.setRedBufferSize(8);
//    ftm.setGreenBufferSize(8);
//    ftm.setBlueBufferSize(8);
//    ftm.setAlphaBufferSize(8);
//    ftm.setDepthBufferSize(24);
//    ftm.setStencilBufferSize(8);
//    ftm.setSwapInterval(0);
//    QSurfaceFormat::setDefaultFormat(ftm);

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // This line is only necessary for QuickVR examples. Once QuickVR is
    // deployed/installed on your system, the application sould not have any
    // trouble finding QuickVR/quickvrplugin.dll or QuickVR/libquickvrplugin.so
    engine.addImportPath(app.applicationDirPath() + "/../../../src/imports");

    engine.load("qrc:/main.qml");

    return app.exec();
}
