#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QtQml/QQmlExtensionPlugin>

#include <beamr/config.h>
#include <beamr/log.h>

Q_IMPORT_QML_PLUGIN(Beamr_ReceiverPlugin)

int main(int argc, char *argv[])
{
    beamr::installLogPattern();

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("beamr-receiver"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("beamr"));
    QGuiApplication::setOrganizationName(QStringLiteral("beamr"));
    QGuiApplication::setApplicationVersion(QString::fromLatin1(beamr::kVersion));
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/beamr/beamr.png")));
    // Lets Wayland and X11 docks match the window to beamr.desktop.
    QGuiApplication::setDesktopFileName(QStringLiteral("beamr"));

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.loadFromModule("Beamr.Receiver", "Main");

    qCInfo(lcBeamr) << "receiver" << beamr::kVersion << "started";
    return app.exec();
}
