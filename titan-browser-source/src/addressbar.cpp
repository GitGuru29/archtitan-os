#include "addressbar.h"
#include <QFocusEvent>
#include <QKeyEvent>
#include <QUrl>

AddressBar::AddressBar(QWidget *parent) : QLineEdit(parent)
{
    setPlaceholderText(QStringLiteral("Search or enter address…"));
    setClearButtonEnabled(true);
    setStyleSheet(QStringLiteral(R"(
        QLineEdit {
            background: rgba(255,255,255,0.05);
            border: 1px solid rgba(122,162,247,0.2);
            border-radius: 10px;
            color: #c0caf5;
            padding: 4px 12px;
            font-size: 12px;
            selection-background-color: rgba(122,162,247,0.3);
        }
        QLineEdit:focus {
            background: rgba(122,162,247,0.08);
            border-color: rgba(122,162,247,0.45);
            box-shadow: 0 0 10px rgba(122,162,247,0.15);
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
