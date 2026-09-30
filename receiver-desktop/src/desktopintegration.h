#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

class QMenu;
class QSystemTrayIcon;

// Living alongside the desktop: a tray icon, staying reachable with the
// window closed, notifications while hidden, and starting at sign-in.
class DesktopIntegration : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // False where the desktop has no tray: closing the window then quits.
    Q_PROPERTY(bool trayAvailable READ trayAvailable CONSTANT)
    // Close hides the window to the tray instead of quitting.
    Q_PROPERTY(bool keepRunning READ keepRunning WRITE setKeepRunning NOTIFY keepRunningChanged)
    Q_PROPERTY(bool launchAtLogin READ launchAtLogin WRITE setLaunchAtLogin NOTIFY launchAtLoginChanged)
    // Started at sign-in: begin hidden in the tray.
    Q_PROPERTY(bool startHidden READ startHidden CONSTANT)

public:
    explicit DesktopIntegration(QObject *parent = nullptr);
    ~DesktopIntegration() override;

    bool trayAvailable() const;
    bool keepRunning() const { return m_keepRunning; }
    void setKeepRunning(bool keep);
    bool launchAtLogin() const;
    void setLaunchAtLogin(bool launch);
    bool startHidden() const { return m_startHidden; }

    // A system notification, e.g. a phone asking while the window is hidden.
    Q_INVOKABLE void notify(const QString &title, const QString &message);
    Q_INVOKABLE void quit();

signals:
    void keepRunningChanged();
    void launchAtLoginChanged();
    // The tray icon, its menu or a notification was clicked.
    void showRequested();

private:
    void updateQuitBehaviour();

    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_menu = nullptr;
    bool m_keepRunning = true;
    bool m_startHidden = false;
};
