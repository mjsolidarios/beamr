#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTranslator>

#include <beamr/config.h>
#include <beamr/log.h>

int main(int argc, char *argv[])
{
    beamr::installLogPattern();

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("beamr"));
    QGuiApplication::setOrganizationName(QStringLiteral("beamr"));
    QGuiApplication::setApplicationVersion(QString::fromLatin1(beamr::kVersion));

    // The phone's or computer's language, when beamr has been translated into it.
    QTranslator translator;
    if (translator.load(QLocale(), QStringLiteral("beamr_sender"), QStringLiteral("_"), QStringLiteral(":/i18n")))
        QCoreApplication::installTranslator(&translator);

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
