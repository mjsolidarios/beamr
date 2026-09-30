#pragma once

#include <QColor>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

#include <vector>

// Draws `text` as a QR code, crisp at any size. Leaves the quiet zone to
// whatever it sits on.
class QrCodeItem : public QQuickPaintedItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(QrCode)

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    explicit QrCodeItem(QQuickItem *parent = nullptr);

    QString text() const { return m_text; }
    void setText(const QString &text);
    QColor color() const { return m_color; }
    void setColor(const QColor &color);

    void paint(QPainter *painter) override;

signals:
    void textChanged();
    void colorChanged();

private:
    QString m_text;
    QColor m_color = Qt::black;
    int m_size = 0;
    std::vector<bool> m_modules;
};
