#include "desktopintegration.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QSettings>
#include <QStandardPaths>
#include <QSystemTrayIcon>

#include <beamr/log.h>

namespace {

const QString kKeepRunningKey = QStringLiteral("app/keepRunning");
const QString kBackgroundArg = QStringLiteral("--background");

// What sign-in should run: the AppImage itself when running from one (its
// contents live in a temporary mount), else this executable.
QString launchCommand()
{
    const QString appImage = qEnvironmentVariable("APPIMAGE");
    return appImage.isEmpty() ? QCoreApplication::applicationFilePath() : appImage;
}

#if defined(Q_OS_WIN)
QSettings runKey()
{
    return QSettings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                     QSettings::NativeFormat);
}
#elif defined(Q_OS_MACOS)
QString launchAgentPath()
{
    return QDir::homePath() + QStringLiteral("/Library/LaunchAgents/io.github.mjsolidarios.beamr.plist");
}
#else
QString autostartPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/autostart/beamr.desktop");
}
#endif

} // namespace

DesktopIntegration::DesktopIntegration(QObject *parent)
    : QObject(parent)
    , m_startHidden(QCoreApplication::arguments().contains(kBackgroundArg))
{
    m_keepRunning = QSettings().value(kKeepRunningKey, true).toBool();

    if (trayAvailable()) {
        m_menu = new QMenu;
        m_menu->addAction(tr("Show beamr"), this, &DesktopIntegration::showRequested);
        m_menu->addSeparator();
        m_menu->addAction(tr("Quit beamr"), this, &DesktopIntegration::quit);

        m_tray = new QSystemTrayIcon(QApplication::windowIcon(), this);
        m_tray->setToolTip(QStringLiteral("beamr"));
        m_tray->setContextMenu(m_menu);
        connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
                emit showRequested();
        });
        connect(m_tray, &QSystemTrayIcon::messageClicked, this, &DesktopIntegration::showRequested);
        m_tray->show();
    } else {
        // Nowhere to live without a window, and nothing started hidden can
        // be reached.
        m_startHidden = false;
    }
    updateQuitBehaviour();
}

DesktopIntegration::~DesktopIntegration()
{
    delete m_menu;
}

bool DesktopIntegration::trayAvailable() const
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void DesktopIntegration::setKeepRunning(bool keep)
{
    if (keep == m_keepRunning)
        return;
    m_keepRunning = keep;
    QSettings().setValue(kKeepRunningKey, keep);
    updateQuitBehaviour();
    emit keepRunningChanged();
}

void DesktopIntegration::updateQuitBehaviour()
{
    // Pop-out windows are windows too: closing the last one must not quit
    // while beamr lives in the tray.
    QApplication::setQuitOnLastWindowClosed(!(m_keepRunning && m_tray));
}

bool DesktopIntegration::launchAtLogin() const
{
#if defined(Q_OS_WIN)
    return runKey().contains(QStringLiteral("beamr"));
#elif defined(Q_OS_MACOS)
    return QFile::exists(launchAgentPath());
#else
    return QFile::exists(autostartPath());
#endif
}

void DesktopIntegration::setLaunchAtLogin(bool launch)
{
    if (launch == launchAtLogin())
        return;
    const QString command = launchCommand();
#if defined(Q_OS_WIN)
    QSettings key = runKey();
    if (launch)
        key.setValue(QStringLiteral("beamr"),
                     QStringLiteral("\"%1\" %2").arg(QDir::toNativeSeparators(command), kBackgroundArg));
    else
        key.remove(QStringLiteral("beamr"));
#else
#if defined(Q_OS_MACOS)
    const QString path = launchAgentPath();
    const QString contents = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
        "<plist version=\"1.0\"><dict>\n"
        "  <key>Label</key><string>io.github.mjsolidarios.beamr</string>\n"
        "  <key>ProgramArguments</key><array><string>%1</string><string>%2</string></array>\n"
        "  <key>RunAtLoad</key><true/>\n"
        "</dict></plist>\n").arg(command.toHtmlEscaped(), kBackgroundArg);
#else
    const QString path = autostartPath();
    const QString contents = QStringLiteral(
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=beamr\n"
        "Comment=Show your Android phone's screen on this computer\n"
        "Exec=\"%1\" %2\n"
        "Icon=beamr\n"
        "X-GNOME-Autostart-enabled=true\n").arg(command, kBackgroundArg);
#endif
    if (launch) {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCWarning(lcBeamr) << "can't write" << path << file.errorString();
            return;
        }
        file.write(contents.toUtf8());
    } else {
        QFile::remove(path);
    }
#endif
    emit launchAtLoginChanged();
}

void DesktopIntegration::notify(const QString &title, const QString &message)
{
    if (m_tray)
        m_tray->showMessage(title, message, QApplication::windowIcon(), 8000);
}

void DesktopIntegration::quit()
{
    QCoreApplication::quit();
}
