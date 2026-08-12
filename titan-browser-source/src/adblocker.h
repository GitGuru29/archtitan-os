#pragma once
#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineUrlRequestInfo>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QAtomicInt>
#include <QRegularExpression>

struct FilterRule {
    enum class Type { DomainExact, DomainSuffix, UrlPattern, Exception };
    Type       type;
    QString    pattern;           // raw text for domain rules
    QRegularExpression regex;     // compiled for URL pattern rules
    bool       thirdPartyOnly = false;
    QSet<int>  resourceTypes;     // QWebEngineUrlRequestInfo::ResourceType values; empty = all
};

class AdBlocker : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT
public:
    explicit AdBlocker(QObject *parent = nullptr);

    // QWebEngineUrlRequestInterceptor interface
    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

    // Stats
    qint64 blockedCount()  const { return m_blocked.loadRelaxed();  }
    qint64 requestCount()  const { return m_requests.loadRelaxed(); }
    void   resetStats()          { m_blocked.storeRelaxed(0); m_requests.storeRelaxed(0); }

    // Dynamic control
    bool isEnabled()        const { return m_enabled; }
    void setEnabled(bool on)      { m_enabled = on; }

    void loadFilterList(const QString &filePath);
    void addRule(const QString &rawRule);

    // Content script injection for YouTube & Spotify & cosmetic ad blocking
    void installContentScript(class QWebEngineProfile *profile);
    void injectContentScriptIntoView(class QWebEngineView *view);
    QString contentScriptSource() const;

private:
    bool isBlocked(const QUrl &url, const QUrl &firstPartyUrl,
                   QWebEngineUrlRequestInfo::ResourceType type) const;

    // Domain blocklist — O(1) lookup
    QSet<QString>       m_blockedDomains;    // exact  e.g. "ads.example.com"
    QSet<QString>       m_blockedBaseDomains;// suffix e.g. "doubleclick.net"

    // URL pattern rules (EasyList-style)
    QList<FilterRule>   m_blockRules;
    QList<FilterRule>   m_exceptionRules;

    QAtomicInt          m_blocked{0};
    QAtomicInt          m_requests{0};
    bool                m_enabled = true;

    void loadBuiltinRules();
    FilterRule parseRule(const QString &line) const;

    // Helpers
    static QString extractHostname(const QUrl &url);
    static QString baseDomain(const QString &host);
    static QRegularExpression buildRegex(QString pattern);
};
