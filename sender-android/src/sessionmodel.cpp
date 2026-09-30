#include "sessionmodel.h"

#include "receiversession.h"

int SessionModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_sessions.size());
}

QVariant SessionModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid))
        return {};
    const ReceiverSession *session = m_sessions.at(index.row());
    switch (role) {
    case SessionIdRole:
        return session->id();
    case NameRole:
        return session->name();
    case AddressRole:
        return session->endpoint();
    case StateRole:
        return int(session->state());
    default:
        return {};
    }
}

QHash<int, QByteArray> SessionModel::roleNames() const
{
    return {
        {SessionIdRole, "sessionId"},
        {NameRole, "name"},
        {AddressRole, "address"},
        {StateRole, "sessionState"},
    };
}

ReceiverSession *SessionModel::find(const QString &sessionId) const
{
    for (ReceiverSession *session : m_sessions) {
        if (session->id() == sessionId)
            return session;
    }
    return nullptr;
}

void SessionModel::add(ReceiverSession *session)
{
    const int row = int(m_sessions.size());
    beginInsertRows({}, row, row);
    m_sessions.append(session);
    endInsertRows();
    connect(session, &ReceiverSession::changed, this, [this, session] { refresh(session); });
    emit countChanged();
}

void SessionModel::remove(ReceiverSession *session)
{
    const int row = int(m_sessions.indexOf(session));
    if (row < 0)
        return;
    beginRemoveRows({}, row, row);
    m_sessions.removeAt(row);
    endRemoveRows();
    session->disconnect(this);
    emit countChanged();
}

void SessionModel::refresh(ReceiverSession *session)
{
    const int row = int(m_sessions.indexOf(session));
    if (row >= 0)
        emit dataChanged(index(row), index(row));
}
