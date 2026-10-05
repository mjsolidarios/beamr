#pragma once

#include <QString>
#include <QVariantList>

// License texts shipped inside both apps, so About works with no network.
namespace beamr::notices {

// THIRD_PARTY_NOTICES.md, with markdown links turned into their labels.
QString text();

// [{id, title}] in display order, notices first.
QVariantList licenses();

// The bundled text for `id`, or empty when `id` is unknown.
QString license(const QString &id);

} // namespace beamr::notices
