#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QtQml/qqmlregistration.h>

#include <optional>

#include <beamr/device.h>

struct ConnectionRequest
{
    QString requestId;
    beamr::DeviceInfo device;
    QString address;
    QDateTime expiresAt;
    bool demo = false;
    // The screen the phone asked for (from its QR code); empty for any.
    QString screenId;
    // The QR code's one-time pairing code, and the code for coming back
    // after a drop; see beamr/protocol.h.
    QString pair;
    QString resume;
};

// Senders waiting for the user to allow or decline their cast.
class ConnectionRequestModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by ReceiverController")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Role {
        RequestIdRole = Qt::UserRole + 1,
        DeviceNameRole,
        DeviceModelRole,
        AddressRole,
        SecondsLeftRole,
        ScreenIdRole,
    };

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // A second request from the same device replaces the first.
    void add(const ConnectionRequest &request);
    std::optional<ConnectionRequest> find(const QString &requestId) const;
    Q_INVOKABLE QString deviceNameAt(int row) const;
    std::optional<ConnectionRequest> take(const QString &requestId);
    QList<ConnectionRequest> takeExpired(const QDateTime &now);
    void refreshCountdowns();

signals:
    void countChanged();

private:
    void removeAt(int row);

    QList<ConnectionRequest> m_requests;
};
