#include <beamr/notices.h>

#include <QFile>
#include <QRegularExpression>
#include <QVariantMap>
#include <QtCore/qtresource.h>

// Q_INIT_RESOURCE declares a global function. Inside a namespace the
// declaration would be a different function and the link would fail.
static void initNotices()
{
    Q_INIT_RESOURCE(beamr_notices);
}

namespace beamr::notices {

namespace {

struct Entry
{
    const char *id;
    const char *title;
    const char *path; // empty for the notices text itself
};

// Resource paths under the beamr_notices qrc (prefix /beamr/notices).
const Entry kLicenses[] = {
    {"notices", "Third-party notices", ""},
    {"mit", "beamr (MIT)", ":/beamr/notices/LICENSE"},
    {"lgpl-3.0", "GNU LGPL v3 (Qt)", ":/beamr/notices/licenses/LGPL-3.0.txt"},
    {"gpl-3.0", "GNU GPL v3", ":/beamr/notices/licenses/GPL-3.0.txt"},
    {"lgpl-2.1", "FFmpeg (LGPL v2.1)", ":/beamr/notices/third_party/ffmpeg/COPYING.LGPLv2.1"},
    {"androidx", "AndroidX (Apache 2.0)", ":/beamr/notices/licenses/Apache-2.0.txt"},
    {"coroutines", "Kotlin coroutines (Apache 2.0)", ":/beamr/notices/licenses/Apache-2.0.txt"},
    {"libcxx", "libc++ (Apache 2.0 with LLVM Exceptions)", ":/beamr/notices/licenses/LLVM-libc++.txt"},
    {"lucide", "Lucide icons (ISC)", ":/beamr/notices/licenses/Lucide-ISC.txt"},
    {"micro-ecc", "micro-ecc (BSD 2-clause)", ":/beamr/notices/third_party/micro-ecc/LICENSE.txt"},
};

void ensureLoaded()
{
    // The resource object lives in this static library; an unreferenced
    // initializer would be dropped at link time.
    initNotices();
}

QString readResource(const char *path)
{
    ensureLoaded();
    QFile file(QString::fromLatin1(path));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(file.readAll());
}

} // namespace

QString text()
{
    QString notices = readResource(":/beamr/notices/THIRD_PARTY_NOTICES.md");
    static const QRegularExpression link(QStringLiteral("\\[([^\\]]+)\\]\\([^)]*\\)"));
    notices.replace(link, QStringLiteral("\\1"));
    return notices;
}

QVariantList licenses()
{
    ensureLoaded();
    QVariantList list;
    for (const Entry &entry : kLicenses)
        list.append(QVariantMap{{QStringLiteral("id"), QString::fromLatin1(entry.id)},
                                {QStringLiteral("title"), QString::fromUtf8(entry.title)}});
    return list;
}

QString license(const QString &id)
{
    for (const Entry &entry : kLicenses) {
        if (id != QLatin1StringView(entry.id))
            continue;
        if (entry.path[0] == '\0')
            return text();
        return readResource(entry.path);
    }
    return {};
}

} // namespace beamr::notices
