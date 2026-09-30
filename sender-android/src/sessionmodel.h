#pragma once

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

class ReceiverSession;

// The receivers the phone is connected to, for the list on the home screen.
// SenderController owns the sessions; this only shows them.
class SessionModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by SenderController")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Role {
        SessionIdRole = Qt::UserRole + 1,
        NameRole,
        AddressRole,
        StateRole,
    };

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    const QList<ReceiverSession *> &sessions() const { return m_sessions; }
    ReceiverSession *find(const QString &sessionId) const;
    void add(ReceiverSession *session);
    void remove(ReceiverSession *session);

signals:
    void countChanged();

private:
    void refresh(ReceiverSession *session);

    QList<ReceiverSession *> m_sessions;
};
