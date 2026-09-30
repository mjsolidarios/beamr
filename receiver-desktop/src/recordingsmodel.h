#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QFileSystemWatcher>
#include <QtQml/qqmlregistration.h>

// Video files in the recordings folder, newest first. Follows changes made
// outside the app too.
class RecordingsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by ReceiverController")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Role {
        FileNameRole = Qt::UserRole + 1,
        FileUrlRole,
        SizeTextRole,
        DateTextRole,
    };

    explicit RecordingsModel(const QString &directory, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filePath(int row) const;
    QString fileName(int row) const;

    Q_INVOKABLE void refresh();

signals:
    void countChanged();

private:
    struct Entry
    {
        QString name;
        QString path;
        qint64 size = 0;
        QDateTime modified;
    };

    QString m_directory;
    QList<Entry> m_entries;
    QFileSystemWatcher m_watcher;
};
