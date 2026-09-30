#include "connectionrequestmodel.h"

int ConnectionRequestModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_requests.size());
}

QVariant ConnectionRequestModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid))
        return {};

    const ConnectionRequest &request = m_requests.at(index.row());
    switch (role) {
    case RequestIdRole:
        return request.requestId;
    case DeviceNameRole:
        return request.device.name;
    case DeviceModelRole:
        return request.device.model;
    case AddressRole:
        return request.address;
    case SecondsLeftRole:
        return qMax<qint64>(0, QDateTime::currentDateTime().secsTo(request.expiresAt));
    case ScreenIdRole:
        return request.screenId;
    }
    return {};
}

QHash<int, QByteArray> ConnectionRequestModel::roleNames() const
{
    return {
        {RequestIdRole, "requestId"},
        {DeviceNameRole, "deviceName"},
        {DeviceModelRole, "deviceModel"},
        {AddressRole, "address"},
        {SecondsLeftRole, "secondsLeft"},
        {ScreenIdRole, "screenId"},
    };
}

void ConnectionRequestModel::add(const ConnectionRequest &request)
{
    for (int row = 0; row < m_requests.size(); ++row) {
        if (m_requests.at(row).device.id == request.device.id) {
            m_requests[row] = request;
            emit dataChanged(index(row), index(row));
            return;
        }
    }

    const int row = int(m_requests.size());
    beginInsertRows({}, row, row);
    m_requests.append(request);
    endInsertRows();
    emit countChanged();
}

std::optional<ConnectionRequest> ConnectionRequestModel::find(const QString &requestId) const
{
    for (const ConnectionRequest &request : m_requests) {
        if (request.requestId == requestId)
            return request;
    }
    return std::nullopt;
}

std::optional<ConnectionRequest> ConnectionRequestModel::take(const QString &requestId)
{
    for (int row = 0; row < m_requests.size(); ++row) {
        if (m_requests.at(row).requestId == requestId) {
            ConnectionRequest request = m_requests.at(row);
            removeAt(row);
            return request;
        }
    }
    return std::nullopt;
}

QList<ConnectionRequest> ConnectionRequestModel::takeExpired(const QDateTime &now)
{
    QList<ConnectionRequest> expired;
    for (int row = int(m_requests.size()) - 1; row >= 0; --row) {
        if (m_requests.at(row).expiresAt <= now) {
            expired.prepend(m_requests.at(row));
            removeAt(row);
        }
    }
    return expired;
}

void ConnectionRequestModel::refreshCountdowns()
{
    if (!m_requests.isEmpty())
        emit dataChanged(index(0), index(int(m_requests.size()) - 1), {SecondsLeftRole});
}

void ConnectionRequestModel::removeAt(int row)
{
    beginRemoveRows({}, row, row);
    m_requests.removeAt(row);
    endRemoveRows();
    emit countChanged();
}
