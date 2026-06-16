#pragma once

#include <QWidget>
#include <QPoint>
#include <QMouseEvent>
#include <QPaintEvent>

class ProgressBar;

class Gui final : public QWidget {
    Q_OBJECT
public:
    explicit Gui(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    QPoint m_dragPos;
    QWidget *buildCard(const QString &title, QWidget *parent);
    QWidget *buildRow(const QString &icon, const QString &key, const QString &value, QWidget *parent);
    QWidget *buildProgressRow(const QString &icon, const QString &key,
                              long used, long total, const QString &unit, QWidget *parent);
};
