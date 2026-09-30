#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <beamr/config.h>
#include <beamr/log.h>

int main(int argc, char *argv[])
{
    beamr::installLogPattern();

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("beamr"));
    QGuiApplication::setOrganizationName(QStringLiteral("beamr"));
    QGuiApplication::setApplicationVersion(QString::fromLatin1(beamr::kVersion));

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {QStringLiteral("appVersion"), QString::fromLatin1(beamr::kVersion)},
    });
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.loadFromModule("Beamr.Sender", "Main");

    qCInfo(lcBeamr) << "sender" << beamr::kVersion << "started";
    return app.exec();
}
