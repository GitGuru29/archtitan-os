#pragma once
#include <QLineEdit>

class AddressBar : public QLineEdit
{
    Q_OBJECT
public:
    explicit AddressBar(QWidget *parent = nullptr);
    void setUrl(const QUrl &url);

signals:
    void urlEntered(const QUrl &url);

protected:
    void focusInEvent(QFocusEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void onReturnPressed();
};
