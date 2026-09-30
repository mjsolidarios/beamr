#include "recordingsmodel.h"

#include <QDir>
#include <QLocale>
#include <QUrl>

RecordingsModel::RecordingsModel(const QString &directory, QObject *parent)
    : QAbstractListModel(parent)
    , m_directory(directory)
{
    QDir().mkpath(m_directory);
    m_watcher.addPath(m_directory);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &RecordingsModel::refresh);
    refresh();
}

int RecordingsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_entries.size());
}

QVariant RecordingsModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid))
        return {};

    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case FileNameRole:
        return entry.name;
    case FileUrlRole:
        return QUrl::fromLocalFile(entry.path);
    case SizeTextRole:
        return QLocale().formattedDataSize(entry.size);
    case DateTextRole:
        return QLocale().toString(entry.modified, QLocale::ShortFormat);
    }
    return {};
}

QHash<int, QByteArray> RecordingsModel::roleNames() const
{
    return {
        {FileNameRole, "fileName"},
        {FileUrlRole, "fileUrl"},
        {SizeTextRole, "sizeText"},
        {DateTextRole, "dateText"},
    };
}

QString RecordingsModel::filePath(int row) const
{
    return row >= 0 && row < m_entries.size() ? m_entries.at(row).path : QString();
}

QString RecordingsModel::fileName(int row) const
{
    return row >= 0 && row < m_entries.size() ? m_entries.at(row).name : QString();
}

void RecordingsModel::refresh()
{
    const QFileInfoList files = QDir(m_directory).entryInfoList(
        {QStringLiteral("*.mkv"), QStringLiteral("*.mp4"), QStringLiteral("*.ts")},
        QDir::Files, QDir::Time);

    const int oldCount = int(m_entries.size());
    beginResetModel();
    m_entries.clear();
    for (const QFileInfo &file : files)
        m_entries.append({file.fileName(), file.absoluteFilePath(), file.size(), file.lastModified()});
    endResetModel();

    if (m_entries.size() != oldCount)
        emit countChanged();
}
