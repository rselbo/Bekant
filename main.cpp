#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "bekant.h"
#include "memorypositionmodel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName("Bekant controller");
    QCoreApplication::setOrganizationDomain("github.com/rselbo/bekant");
    QCoreApplication::setOrganizationName("Bekant");

    Bekant bekant;
    QQuickStyle::setStyle("Material");

    QQmlApplicationEngine engine;
    const QUrl url(u"qrc:/Bekant/main.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    qmlRegisterType<MemoryPositionModel>("Bekant", 1, 0, "MemoryPositionModel");

    engine.rootContext()->setContextProperty("bekant", &bekant);
    engine.load(url);

    return app.exec();
}
