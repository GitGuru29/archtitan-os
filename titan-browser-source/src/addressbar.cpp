#include "addressbar.h"
#include <QFocusEvent>
#include <QKeyEvent>
#include <QAction>
#include <QIcon>
#include <QUrlQuery>

AddressBar::AddressBar(QWidget *parent) : QLineEdit(parent)
{
    setPlaceholderText(QStringLiteral("Search web (or type 'gh ', 'yt ', 'wiki ')..."));
    setClearButtonEnabled(true);

    // Leading security / search badge
    m_securityAction = addAction(QIcon(QStringLiteral(":/icons/search.svg")), QLineEdit::LeadingPosition);
    m_securityAction->setToolTip(QStringLiteral("Search or enter web address"));

    // Trailing bookmark star action
    m_bookmarkAction = addAction(QIcon(QStringLiteral(":/icons/star.svg")), QLineEdit::TrailingPosition);
    m_bookmarkAction->setToolTip(QStringLiteral("Bookmark this page"));
    connect(m_bookmarkAction, &QAction::triggered, this, &AddressBar::onBookmarkActionTriggered);

    setStyleSheet(QStringLiteral(R"(
        QLineEdit {
            background: #091022;
            border: 1px solid rgba(56, 189, 248, 0.18);
            border-radius: 8px;
            color: #f1f5f9;
            padding: 4px 10px 4px 8px;
            font-size: 13px;
            font-family: "Inter", -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
            selection-background-color: rgba(56, 189, 248, 0.35);
            min-height: 26px;
        }
        QLineEdit:hover {
            background: #0c152a;
            border-color: rgba(56, 189, 248, 0.35);
        }
        QLineEdit:focus {
            background: #0e1a34;
            border: 1px solid #38bdf8;
            color: #ffffff;
        }
    )"));

    connect(this, &QLineEdit::returnPressed, this, &AddressBar::onReturnPressed);
    connect(this, &QLineEdit::textChanged, this, &AddressBar::onTextChanged);
}

void AddressBar::setUrl(const QUrl &url)
{
    m_currentUrl = url;
    if (!hasFocus()) {
        if (url.toString() == QStringLiteral("qrc:/homepage.html") || url.isEmpty()) {
            clear();
        } else if (url.toString() == QStringLiteral("qrc:/settings.html")) {
            setText(QStringLiteral("titan://settings"));
        } else {
            setText(url.toString());
        }
    }
    updateSecurityBadge(url);
}

void AddressBar::setBookmarked(bool bookmarked)
{
    m_isBookmarked = bookmarked;
    if (m_bookmarkAction) {
        if (m_isBookmarked) {
            m_bookmarkAction->setIcon(QIcon(QStringLiteral(":/icons/star-filled.svg")));
            m_bookmarkAction->setToolTip(QStringLiteral("Bookmarked (click to remove)"));
        } else {
            m_bookmarkAction->setIcon(QIcon(QStringLiteral(":/icons/star.svg")));
            m_bookmarkAction->setToolTip(QStringLiteral("Bookmark this page"));
        }
    }
}

void AddressBar::updateSecurityBadge(const QUrl &url)
{
    if (!m_securityAction) return;

    QString scheme = url.scheme().toLower();
    if (scheme == QStringLiteral("https")) {
        m_securityAction->setIcon(QIcon(QStringLiteral(":/icons/lock.svg")));
        m_securityAction->setToolTip(QStringLiteral("Secure Connection (HTTPS verified)"));
    } else if (scheme == QStringLiteral("http")) {
        m_securityAction->setIcon(QIcon(QStringLiteral(":/icons/lock-open.svg")));
        m_securityAction->setToolTip(QStringLiteral("Warning: Not Secure (Unencrypted HTTP connection)"));
    } else {
        m_securityAction->setIcon(QIcon(QStringLiteral(":/icons/search.svg")));
        m_securityAction->setToolTip(QStringLiteral("Search or enter web address"));
    }
}

void AddressBar::onBookmarkActionTriggered()
{
    emit bookmarkClicked(m_currentUrl);
}

void AddressBar::focusInEvent(QFocusEvent *e)
{
    QLineEdit::focusInEvent(e);
    selectAll();
}

void AddressBar::focusOutEvent(QFocusEvent *e)
{
    QLineEdit::focusOutEvent(e);
    if (!m_currentUrl.isEmpty() && m_currentUrl.toString() != QStringLiteral("qrc:/homepage.html")) {
        setUrl(m_currentUrl);
    }
}

void AddressBar::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        if (!m_currentUrl.isEmpty()) {
            setUrl(m_currentUrl);
        }
        clearFocus();
        return;
    }
    QLineEdit::keyPressEvent(e);
}

void AddressBar::onTextChanged(const QString &text)
{
    // If text starts with special search shortcuts, update hint
    QString trimmed = text.trimmed();
    if (trimmed.startsWith(QStringLiteral("gh "))) {
        setPlaceholderText(QStringLiteral("Search GitHub repositories & code..."));
    } else if (trimmed.startsWith(QStringLiteral("yt "))) {
        setPlaceholderText(QStringLiteral("Search YouTube videos..."));
    } else if (trimmed.startsWith(QStringLiteral("wiki "))) {
        setPlaceholderText(QStringLiteral("Search Wikipedia encyclopedia..."));
    } else {
        setPlaceholderText(QStringLiteral("Search web (or type 'gh ', 'yt ', 'wiki ')..."));
    }
}

void AddressBar::setSearchEngine(const QString &engine)
{
    m_searchEngine = engine.trimmed().toLower();
    if (m_searchEngine.isEmpty()) m_searchEngine = QStringLiteral("google");
}

QUrl AddressBar::buildSearchUrl(const QString &query) const
{
    QString engine = m_searchEngine.toLower();
    QString encoded = QString::fromUtf8(QUrl::toPercentEncoding(query));

    if (engine == QStringLiteral("duckduckgo") || engine == QStringLiteral("ddg")) {
        return QUrl(QStringLiteral("https://duckduckgo.com/?q=") + encoded);
    } else if (engine == QStringLiteral("brave")) {
        return QUrl(QStringLiteral("https://search.brave.com/search?q=") + encoded);
    } else if (engine == QStringLiteral("bing")) {
        return QUrl(QStringLiteral("https://www.bing.com/search?q=") + encoded);
    } else if (engine == QStringLiteral("kagi")) {
        return QUrl(QStringLiteral("https://kagi.com/search?q=") + encoded);
    } else if (engine == QStringLiteral("ecosia")) {
        return QUrl(QStringLiteral("https://www.ecosia.org/search?q=") + encoded);
    } else if (engine == QStringLiteral("yahoo")) {
        return QUrl(QStringLiteral("https://search.yahoo.com/search?p=") + encoded);
    } else {
        // Default Google Search
        return QUrl(QStringLiteral("https://www.google.com/search?q=") + encoded);
    }
}

void AddressBar::onReturnPressed()
{
    QString input = text().trimmed();
    if (input.isEmpty()) return;

    QUrl url;

    // Quick command aliases (Prefix shortcuts)
    if (input.startsWith(QStringLiteral("gh "))) {
        QString q = input.mid(3).trimmed();
        url = QUrl(QStringLiteral("https://github.com/search?q=") + QUrl::toPercentEncoding(q));
    } else if (input.startsWith(QStringLiteral("yt "))) {
        QString q = input.mid(3).trimmed();
        url = QUrl(QStringLiteral("https://www.youtube.com/results?search_query=") + QUrl::toPercentEncoding(q));
    } else if (input.startsWith(QStringLiteral("wiki "))) {
        QString q = input.mid(5).trimmed();
        url = QUrl(QStringLiteral("https://en.wikipedia.org/wiki/Special:Search?search=") + QUrl::toPercentEncoding(q));
    } else if (input.startsWith(QStringLiteral("g "))) {
        QString q = input.mid(2).trimmed();
        url = QUrl(QStringLiteral("https://www.google.com/search?q=") + QUrl::toPercentEncoding(q));
    } else if (input.startsWith(QStringLiteral("ddg "))) {
        QString q = input.mid(4).trimmed();
        url = QUrl(QStringLiteral("https://duckduckgo.com/?q=") + QUrl::toPercentEncoding(q));
    } else if (input.startsWith(QStringLiteral("b ")) || input.startsWith(QStringLiteral("brave "))) {
        int off = input.startsWith(QStringLiteral("b ")) ? 2 : 6;
        QString q = input.mid(off).trimmed();
        url = QUrl(QStringLiteral("https://search.brave.com/search?q=") + QUrl::toPercentEncoding(q));
    } else if (input.startsWith(QStringLiteral("bing "))) {
        QString q = input.mid(5).trimmed();
        url = QUrl(QStringLiteral("https://www.bing.com/search?q=") + QUrl::toPercentEncoding(q));
    } else if (input == QStringLiteral("titan://settings") || input == QStringLiteral("settings")) {
        url = QUrl(QStringLiteral("qrc:/settings.html"));
    } else if (input == QStringLiteral("titan://home") || input == QStringLiteral("home")) {
        url = QUrl(QStringLiteral("qrc:/homepage.html"));
    } else if (!input.contains(QStringLiteral("://")) &&
               (input.contains(u'.') || input.startsWith(QStringLiteral("localhost")) || input.contains(u':')) &&
               !input.contains(u' '))
    {
        url = QUrl(QStringLiteral("https://") + input);
    } else if (input.contains(QStringLiteral("://"))) {
        url = QUrl(input);
    } else {
        url = buildSearchUrl(input);
    }

    emit urlEntered(url);
    clearFocus();
}
