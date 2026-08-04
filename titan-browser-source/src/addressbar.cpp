#include "addressbar.h"
#include <QFocusEvent>
#include <QKeyEvent>
#include <QUrl>

AddressBar::AddressBar(QWidget *parent) : QLineEdit(parent)
{
    setPlaceholderText(QStringLiteral("Search or enter address"));
    setClearButtonEnabled(true);

    // Leading search icon
    QAction *searchIcon = addAction(QIcon(), QLineEdit::LeadingPosition);
    searchIcon->setText(QStringLiteral("🔍"));

    setStyleSheet(QStringLiteral(R"(
        QLineEdit {
            background: #0d1322;
            border: 1px solid rgba(56, 189, 248, 0.15);
            border-radius: 20px;
            color: #e2e8f0;
            padding: 6px 16px 6px 12px;
            font-size: 13px;
            selection-background-color: rgba(56, 189, 248, 0.3);
            min-height: 22px;
        }
        QLineEdit:focus {
            background: #111827;
            border: 1px solid rgba(56, 189, 248, 0.5);
            box-shadow: 0 0 12px rgba(56, 189, 248, 0.15);
        }
    )"));

    connect(this, &QLineEdit::returnPressed, this, &AddressBar::onReturnPressed);
}

void AddressBar::setUrl(const QUrl &url)
{
    if (!hasFocus())
        setText(url.toString());
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
    // Bare domain / IP → prepend https://
    if (!input.contains(QStringLiteral("://")) &&
        (input.contains(u'.') || input.startsWith(QStringLiteral("localhost"))))
    {
        url = QUrl(QStringLiteral("https://") + input);
    } else if (input.contains(QStringLiteral("://"))) {
        url = QUrl(input);
    } else {
        // Treat as DuckDuckGo search
        url = QUrl(QStringLiteral("https://duckduckgo.com/?q=") +
                   QUrl::toPercentEncoding(input));
    }

    emit urlEntered(url);
}
