#include <beamr/connectlink.h>

#include <QUrl>
#include <QUrlQuery>

namespace beamr {

namespace {
constexpr QLatin1StringView kScheme("beamr");
constexpr QLatin1StringView kAction("connect");
} // namespace

QString ConnectLink::toString() const
{
    QUrlQuery query;
    if (!name.isEmpty())
        query.addQueryItem(QStringLiteral("name"), QString::fromUtf8(QUrl::toPercentEncoding(name)));
    query.addQueryItem(QStringLiteral("port"), QString::number(port));
    if (!screen.isEmpty())
        query.addQueryItem(QStringLiteral("screen"), screen);
    if (!pair.isEmpty())
        query.addQueryItem(QStringLiteral("pair"), pair);
    for (const QString &host : hosts)
        query.addQueryItem(QStringLiteral("host"), host);

    QUrl url;
    url.setScheme(kScheme);
    url.setHost(kAction);
    url.setQuery(query);
    return url.toString(QUrl::FullyEncoded);
}

ConnectLink ConnectLink::parse(const QString &text)
{
    const QUrl url(text.trimmed(), QUrl::StrictMode);
    if (!url.isValid() || url.scheme() != kScheme || url.host() != kAction)
        return {};

    const QUrlQuery query(url);
    ConnectLink link;
    link.name = query.queryItemValue(QStringLiteral("name"), QUrl::FullyDecoded);
    link.screen = query.queryItemValue(QStringLiteral("screen"), QUrl::FullyDecoded).left(32);
    link.pair = query.queryItemValue(QStringLiteral("pair"), QUrl::FullyDecoded).left(64);
    link.hosts = query.allQueryItemValues(QStringLiteral("host"), QUrl::FullyDecoded);
    link.hosts.removeAll(QString());
    bool ok = false;
    const uint port = query.queryItemValue(QStringLiteral("port")).toUInt(&ok);
    if (ok && port > 0 && port <= 65535)
        link.port = quint16(port);
    return link;
}

} // namespace beamr
