#pragma once

#include <QWidget>
#include <QPoint>
#include <QMouseEvent>
#include <QPaintEvent>

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
};
