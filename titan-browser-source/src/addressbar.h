#pragma once
#include <QLineEdit>
#include <QUrl>
#include <QAction>
#include <QListWidget>

class AddressBar : public QLineEdit
{
    Q_OBJECT
public:
    explicit AddressBar(QWidget *parent = nullptr);
    void setUrl(const QUrl &url);
    void setBookmarked(bool bookmarked);

signals:
    void urlEntered(const QUrl &url);
    void bookmarkClicked(const QUrl &url);

protected:
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void onReturnPressed();
    void onTextChanged(const QString &text);
    void onBookmarkActionTriggered();

private:
    void updateSecurityBadge(const QUrl &url);

    QAction *m_securityAction = nullptr;
    QAction *m_bookmarkAction = nullptr;
    QUrl     m_currentUrl;
    bool     m_isBookmarked   = false;
};
