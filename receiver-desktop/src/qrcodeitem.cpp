#include "qrcodeitem.h"

#include <QPainter>
#include <QPainterPath>
#include <QQuickWindow>

#include <algorithm>
#include <cmath>

#include <qrcodegen.hpp>

QrCodeItem::QrCodeItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(false);
}

void QrCodeItem::setText(const QString &text)
{
    if (text == m_text)
        return;
    m_text = text;
    m_modules.clear();
    m_size = 0;
    if (!text.isEmpty()) {
        // Medium error correction: survives a glare spot on the screen
        // without making the code too dense to scan from across a desk.
        const auto qr = qrcodegen::QrCode::encodeText(text.toUtf8().constData(), qrcodegen::QrCode::Ecc::MEDIUM);
        m_size = qr.getSize();
        m_modules.resize(size_t(m_size) * m_size);
        for (int y = 0; y < m_size; ++y) {
            for (int x = 0; x < m_size; ++x)
                m_modules[size_t(y) * m_size + x] = qr.getModule(x, y);
        }
    }
    emit textChanged();
    update();
}

void QrCodeItem::setColor(const QColor &color)
{
    if (color == m_color)
        return;
    m_color = color;
    emit colorChanged();
    update();
}

void QrCodeItem::paint(QPainter *painter)
{
    if (m_size == 0)
        return;
    // Whole device pixels per module keep the edges sharp.
    const qreal dpr = window() ? window()->effectiveDevicePixelRatio() : 1.0;
    const qreal side = std::min(width(), height());
    const qreal module = std::max<qreal>(1.0, std::floor(side * dpr / m_size)) / dpr;
    const qreal offset = (side - module * m_size) / 2;

    QPainterPath path;
    for (int y = 0; y < m_size; ++y) {
        for (int x = 0; x < m_size; ++x) {
            if (m_modules[size_t(y) * m_size + x])
                path.addRect(QRectF(offset + x * module, offset + y * module, module, module));
        }
    }
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_color);
    painter->drawPath(path.simplified());
}
