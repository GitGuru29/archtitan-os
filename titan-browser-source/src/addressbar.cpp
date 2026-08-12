#include "addressbar.h"
#include <QFocusEvent>
#include <QKeyEvent>
#include <QAction>
#include <QIcon>

AddressBar::AddressBar(QWidget *parent) : QLineEdit(parent)
{
    setPlaceholderText(QStringLiteral("Search the web, enter URL, or ask Titan AI..."));
    setClearButtonEnabled(true);

    // Leading subtle search icon
    QAction *searchIcon = addAction(QIcon(QStringLiteral(":/icons/search.svg")), QLineEdit::LeadingPosition);
    searchIcon->setToolTip(QStringLiteral("Search or Command Bar"));

    // Trailing subtle bookmark icon
    QAction *bmIcon = addAction(QIcon(QStringLiteral(":/icons/bookmark.svg")), QLineEdit::TrailingPosition);
    bmIcon->setToolTip(QStringLiteral("Bookmark this tab"));

    setStyleSheet(QStringLiteral(R"(
        QLineEdit {
            background: #090e1a;
            border: 1px solid rgba(255, 255, 255, 0.08);
            border-radius: 8px;
            color: #f1f5f9;
            padding: 5px 12px 5px 10px;
            font-size: 13px;
            selection-background-color: rgba(56, 189, 248, 0.35);
            min-height: 24px;
        }
        QLineEdit:hover {
            background: #0d1527;
            border-color: rgba(56, 189, 248, 0.25);
        }
        QLineEdit:focus {
            background: #0f182d;
            border: 1px solid #38bdf8;
            color: #ffffff;
        }
    )"));

    connect(this, &QLineEdit::returnPressed, this, &AddressBar::onReturnPressed);
}

void AddressBar::setUrl(const QUrl &url)
{
    if (!hasFocus()) {
        if (url.toString() == QStringLiteral("qrc:/homepage.html")) {
            clear();
        } else {
            setText(url.toString());
        }
    }
}

void AddressBar::focusInEvent(QFocusEvent *e)
{
    QLineEdit::focusInEvent(e);
    selectAll();
}

void AddressBar::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        clearFocus();
        return;
    }
    QLineEdit::keyPressEvent(e);
}

void AddressBar::onReturnPressed()
{
    QString input = text().trimmed();
    if (input.isEmpty()) return;

    QUrl url;
    if (!input.contains(QStringLiteral("://")) &&
        (input.contains(u'.') || input.startsWith(QStringLiteral("localhost"))))
    {
        url = QUrl(QStringLiteral("https://") + input);
    } else if (input.contains(QStringLiteral("://"))) {
        url = QUrl(input);
    } else {
        url = QUrl(QStringLiteral("https://duckduckgo.com/?q=") +
                   QUrl::toPercentEncoding(input));
    }

    emit urlEntered(url);
}
