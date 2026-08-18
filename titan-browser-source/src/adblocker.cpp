#include "adblocker.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QUrl>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineProfile>
#include <QWebEngineView>

/* ──────────────────────────────────────────────────────────────────────────
   Built-in block list — curated from EasyList, EasyPrivacy, uBlock Origin,
   and Peter Lowe's blocklist. Covers ads, trackers, telemetry, and malware.
   ─────────────────────────────────────────────────────────────────────── */
static const char * const kBuiltinDomains[] = {
    /* ── YouTube & YouTube Music Ad & Telemetry Endpoints ──────────────── */
    "googleads.g.doubleclick.net", "pagead2.googleadservices.com",
    "ad.youtube.com", "ads.youtube.com",
    "static.doubleclick.net",           /* YT in-stream ad status scripts */
    "imasdk.googleapis.com",            /* YouTube IMA SDK — serves all video ads */


    /* ── Spotify Ad & Telemetry Endpoints ──────────────────────────────────── */
    /* NOTE: these MUST be pure hostnames — no paths allowed here */
    "spclient.wg.spotify.com",        /* ad-logic, gabo telemetry */
    "audio-fa.scdn.co",               /* legacy ad audio CDN */
    "heads-fa.spotify.com",           /* ad-serving endpoint */
    "adeventtracker.spotify.com",     /* ad event tracking */
    "ads-fa.spotify.com",             /* ad delivery */
    "audio-ak-spotify-com.akamaized.net", /* Akamai Spotify ad CDN */
    "seektables.scdn.co",             /* seek data for ads */
    "log.spotify.com",                /* ad event logs */
    "encore.scdn.co",                 /* ad metadata */
    "thisis.spotify.com",             /* Spotify ad promo */


    /* ── Google Advertising ────────────────────────────────────────────── */
    "doubleclick.net", "googleadservices.com", "googlesyndication.com",
    "googletagmanager.com", "googletagservices.com", "google-analytics.com",
    "adservice.google.com", "pagead2.googlesyndication.com", "adwords.google.com",
    "ads.google.com", "adsystem.amazon.com", "advertising.amazon.com",

    /* ── Facebook / Meta Tracking ──────────────────────────────────────── */
    "connect.facebook.net", "facebook.com/tr", "graph.facebook.com",
    "staticxx.facebook.com", "pixel.facebook.com",

    /* ── Major Ad Networks ─────────────────────────────────────────────── */
    "ads.yahoo.com", "advertising.com", "adbrite.com", "adcolony.com",
    "adform.net", "adition.com", "adjug.com", "adnxs.com", "adroll.com",
    "adsafeprotected.com", "adsenseformobileapps.com", "adsrvr.org",
    "adtech.de", "adtechus.com", "adthorize.com", "adtrafficquality.google",
    "adxpansion.com", "amazon-adsystem.com", "appnexus.com", "adblade.com",
    "bidswitch.net", "bidrocket.com", "betrad.com", "bluekai.com",
    "buysellads.com", "casalemedia.com", "contextweb.com", "cpmstar.com",
    "criteo.com", "criteo.net", "crwdcntrl.net", "demdex.net",
    "districtm.ca", "dotomi.com", "emtv.com", "emxdgt.com",
    "exelator.com", "exponential.com", "eyeota.net",
    "flashtalking.com", "freewheel.tv", "gumgum.com",
    "hotjar.com", "hubspot.com", "indexww.com", "iponweb.net",
    "justpremium.com", "kargo.com", "lijit.com", "liveintent.com",
    "loopme.com", "lotame.com", "magnite.com", "media.net",
    "mediavine.com", "moatads.com", "moatpixel.com", "nativo.com",
    "nexac.com", "ogury.com", "openx.net", "openx.com",
    "outbrain.com", "overture.com", "pangle.io", "pubmatic.com",
    "pulse2.me", "quantserve.com", "quantcast.com", "revcontent.com",
    "rfihub.com", "rlcdn.com", "rhythmone.com", "rubiconproject.com",
    "s3solutions.net", "sizmek.com", "smartadserver.com", "sonobi.com",
    "spotxchange.com", "sprinklr.com", "synacormedia.com", "taboola.com",
    "tapad.com", "teads.tv", "themediagrid.com", "tidaltv.com",
    "tribalfusion.com", "triplelift.com", "truste.com", "turn.com",
    "twiago.com", "undertone.com", "unrulymedia.com", "valueclick.com",
    "videology.com", "viglink.com", "w55c.net", "yieldmo.com",
    "yieldmanager.com", "yimg.com", "yieldmanager.edgesuite.net",
    "zedo.com", "zonos.com",

    /* ── Tracking & Analytics ───────────────────────────────────────────── */
    "addthis.com", "addtoany.com", "agkn.com", "amplitude.com",
    "analytics.twitter.com", "analytics.tiktok.com", "api.mixpanel.com",
    "bat.bing.com", "branch.io", "bugsnag.com", "chartbeat.com",
    "chartbeat.net", "chartbeat.com", "clicky.com", "cluster1.eu.amplitude.com",
    "comscore.com", "convertexperiments.com", "crazyegg.com",
    "datadoghq.com", "ddrum5.com", "d.adroll.com", "dgt.ly",
    "disqus.com", "dynatrace.com", "etracker.com", "everesttech.net",
    "exactag.com", "exacttarget.com", "fastly-insights.com",
    "fullstory.com", "ga.jspm.io", "gaug.es", "heapanalytics.com",
    "hm.baidu.com", "hit.gemius.pl", "hits.io", "igodigital.com",
    "imrworldwide.com", "inspectlet.com", "intelliads.com",
    "kissmetrics.com", "kochava.com", "krxd.net",
    "licdn.com", "listrak.com", "livechatinc.com", "livefyre.com",
    "logmein.com", "luckyorange.com", "marketo.net", "marketo.com",
    "mixpanel.com", "monetate.net", "mouseflow.com", "mparticle.com",
    "myfonts.net", "myvisualiq.net", "newrelic.com", "newrelic.js",
    "nr-data.net", "omtrdc.net", "onaudience.com", "optimizely.com",
    "parsely.com", "pingdom.net", "pingdom.com", "piwik.pro",
    "plausible.io", "pointroll.com", "rambler.ru", "revenuehits.com",
    "rtactivate.com", "rxilient.com", "sailthru.com", "salesforce.com",
    "scorecardresearch.com", "searchignite.com", "segment.com",
    "segment.io", "semasio.net", "sharethrough.com", "shopperapproved.com",
    "simpli.fi", "siteimproveanalytics.io", "siteimprove.com",
    "smaato.net", "snoobi.com", "snapwidget.com", "statcounter.com",
    "stickyadstv.com", "tag.appdynamics.com", "tag.clearbit.com",
    "trust.arc.com", "truste.com", "trustpilot.com",
    "unica.com", "update.googleapis.com", "usabilla.com", "userzoom.com",
    "vdna-assets.com", "visualwebsiteoptimizer.com", "voicefive.com",
    "walkme.com", "wootric.com", "yotpo.com", "zopim.com",

    /* ── Telemetry / OS Phoning Home ───────────────────────────────────── */
    "data.microsoft.com", "settings-win.data.microsoft.com",
    "vortex.data.microsoft.com", "telemetry.microsoft.com",
    "telemetry.remobjects.com", "telemetry.yelp.com",
    "app.adjust.com", "app.appsflyer.com", "events.launchdarkly.com",
    "sentry.io", "o1234.ingest.sentry.io",

    /* ── Social Widgets & Tracking ─────────────────────────────────────── */
    "platform.twitter.com", "syndication.twitter.com",
    "cdn.syndication.twimg.com", "apis.google.com",
    "badge.linkedin.com", "platform.linkedin.com",
    "widgets.pinterest.com", "ct.pinterest.com",
    "embed.reddit.com", "reddit-static.com",
    "static.tumblr.com", "embed.tumblr.com",

    /* ── Malware / Phishing domains (common) ───────────────────────────── */
    "adware.guru", "malware-scanner.net", "browser-update.org",
    "freeadultcomics.com", "freegiftcards.pw", "flash-player-update.com",
    "java-update-required.com", "your-computer-is-infected.com",
    "ad.xtendmedia.com", "virus-cleaner.download", "fixmac-pro.com",

    /* ── Coin Mining ────────────────────────────────────────────────────── */
    "coinhive.com", "coin-hive.com", "miner.pr0gramm.com",
    "coin.hive.so", "cryptoloot.pro", "deepminer.io",
    "gus.host", "jsecoin.com", "minecrunch.co",
    "minero.pw", "mining4.us", "monero.io",
    "webmine.cz", "webminer.me",

    /* ── Popup / Redirect abuse ─────────────────────────────────────────── */
    "popads.net", "popmyads.com", "popunder.net", "popunders.net",
    "trafficjunky.net", "ero-advertising.com", "trafficstars.com",
    "traffic.trafik.io", "plugrush.com", "trafficshop.com",
    "zeropark.com", "propellerads.com", "clickadu.com",
    "popunder.ru", "revcpx.com", "adskeeper.com",

    /* ── Fingerprinting as a service ────────────────────────────────────── */
    "fingerprintjs.com", "api.fpjs.io", "cdn.jsdelivr.net/npm/@fingerprintjs",
    "iovation.com", "threatmetrix.com", "kaptcha.com",

    /* ── Misc ad junk ───────────────────────────────────────────────────── */
    "a.adtng.com", "a.tribalfusion.com", "ad.200solutions.com",
    "ad.a8.net", "ad.about.com", "ad.adserverplus.com",
    "ad.auditude.com", "ad.doubleclick.net", "ad.gt",
    "ad.hyena.com.au", "ad.leadbolt.net", "ad.mo.doubleclick.net",
    "ad.turn.com", "ad.wsod.com", "ad.yoc.mobi",
    "ad1.emediate.eu", "ad2.emediate.eu", "adbard.net",
    "adbooth.net", "adcash.com", "adcommunity.net",
    "adconnect.com", "addynamo.net", "adecn.com",
    "adherecreative.com", "adinterax.com", "adjix.com",
    "adlane.com", "admanager.brightcove.com", "admeld.com",
    "adnation.com", "adnetinteractive.com", "adparlor.com",
    "adpepper.com", "adperform.com", "adphorus.com",
    "adpinion.com", "adplugg.com", "adquant.com",
    "adrocket.com", "adsavvy.com", "adscale.de",
    "adserver.adtechus.com", "adserverplus.com", "adsnative.com",
    "adsonar.com", "adspeed.com", "adsq.com",
    "adstanding.com", "adswizz.com", "adtaily.com",
    "adtail.com", "adtomatik.com", "adture.com",
    "adv.emediate.com", "advg.jp", "adviewer.marinsoftware.com",
    "adviva.net", "adwhirl.com", "adwise.com",
    "adwitserver.com", "adworx.at", "adxpansion.com",
    "adyoulike.com", "affiliatly.com", "agoda.net",
    "aimatch.com", "alenty.com", "alien-ads.com",
    "algovast.com", "alkemics.com", "altitude-arena.com",
    "amount-ads.com", "anrdoezrs.net", "anxads.com",
    "ap.lijit.com", "appier.com", "applixir.com",
    "apturl.net", "arrivaltom.com", "arts-ads.com",
    "aswpsdkus.com", "atdmt.com", "audienceiq.com",
    "audiencerate.com", "audiencescience.com", "b.scorecardresearch.com",
    "babs.tv", "banner.meetic.com", "bannersnack.com",
    "beacon.krxd.net", "beacon.securepubads.g.doubleclick.net",
    "belvideo.ru", "bidtellect.com", "bidsystem.io",
    "bigpoint-ads.com", "bnmla.com", "brainrollup.com",
    "brsrvr.com", "btbuckets.com", "bttrack.com",
    "c.betrad.com", "cdn.doubleverify.com", "cdn.iyengar.io",
    "cdn.outbrain.com", "cdn.taboola.com", "cdn.tinyclues.com",
    "cdnads.geeksphone.com", "cdn-ads.com",
    nullptr
};

/* URL-pattern rules (EasyList-compatible subset) */
static const char * const kBuiltinUrlRules[] = {
    /* ── YouTube & YouTube Music In-Stream Ads & Tracking ───────────────── */
    "||youtube.com/api/stats/ads",
    "||youtube.com/pagead/",
    "||youtube.com/ptracking",
    "||youtube.com/youtubei/v1/player/ad_break",
    "||youtube.com/youtubei/v1/player?*adformat*",
    "||youtube.com/get_midroll_info",
    "||youtube.com/api/stats/qoe?*adformat*",
    "||youtube.com/api/stats/atr",
    "||youtube.com/api/stats/watchtime?*adformat*",
    "||youtube.com/api/stats/ads?",
    "||googlevideo.com/videoplayback?*adt=*",    /* YTM audio ad streams */
    "||googleads.g.doubleclick.net/pagead/",
    "||static.doubleclick.net/instream/ad_status.js",
    "||imasdk.googleapis.com/js/sdkloader/",    /* IMA SDK loader */
    "||imasdk.googleapis.com/pal/",             /* IMA PAL (Programmatic Access Library) */
    "||pagead2.googlesyndication.com/pagead/",
    "||ad.doubleclick.net/",


    /* ── Spotify Ads & Tracking ─────────────────────────────────────────────── */
    "||spclient.wg.spotify.com/ads/",
    "||spclient.wg.spotify.com/ad-logic/",
    "||spclient.wg.spotify.com/gabo-receiver-service/",
    "||spclient.wg.spotify.com/ad-event-tracking/",
    "||audio-fa.scdn.co/ad/",
    "||audio-ak-spotify-com.akamaized.net/ad/",
    "||audio4-ak.spotify.com/audio/",
    "||heads-fa.spotify.com/",
    "||adeventtracker.spotify.com/",
    "||ads-fa.spotify.com/",
    "||audio-akp-*.scdn.co/audio/*/ads/",
    "||log.spotify.com/",
    "||seektables.scdn.co/",

    /* ── Web Ad Networks & Trackers ─────────────────────────────────────── */
    "||googlesyndication.com/pagead/",
    "||doubleclick.net/adi/",
    "||doubleclick.net/adj/",
    "||doubleclick.net/pagead/",
    "||ads.twitter.com/",
    "||static.doubleclick.net/",
    "||afs.googlesyndication.com/",
    "||pagead2.googlesyndication.com/pagead/",
    "||adservice.google.*/afs/",
    "||www.googletagmanager.com/gtm.js",
    "||www.googletagmanager.com/gtag/",
    "||bat.bing.com/",
    "||platform.twitter.com/widgets/",
    "||analytics.tiktok.com/",
    "||cdn.ampproject.org/v0/amp-ad-",
    "||securepubads.g.doubleclick.net/",
    "||aax.amazon-adsystem.com/",
    "||s.adroll.com/",
    "||d.adroll.com/",
    "||tag.clearbit.com/",
    "||snap.licdn.com/li.lms-analytics/",
    "||px.ads.linkedin.com/",
    "||tr.snapchat.com/",
    "||analytics.pinterest.com/",
    "||ct.pinterest.com/",
    "||hotjar.com/api/",
    "||widget.hotjar.com/",
    "||static.hotjar.com/",
    "||cdn.segment.com/analytics.js/",
    "||cdn.mxpnl.com/libs/mixpanel",
    "||cdn.amplitude.com/",
    "||assets.adobedtm.com/",
    "||tags.tiqcdn.com/",
    "||tags.bluekai.com/",
    "||cm.g.doubleclick.net/",
    "||cm.g.doubleclick.net/pixel",
    "||b.scorecard research.com/pixel",
    "||beacon.krxd.net/",
    "||events.launchdarkly.com/",
    "||o1234.ingest.sentry.io/",
    "||r.sentry.io/api/",
    "||browser.sentry-cdn.com/",
    "||cdn.coinhive.com/",
    nullptr
};

/* ─── Constructor ──────────────────────────────────────────────────────── */
AdBlocker::AdBlocker(QObject *parent) : QWebEngineUrlRequestInterceptor(parent)
{
    loadBuiltinRules();
}

/* ─── Load built-in lists ─────────────────────────────────────────────── */
void AdBlocker::loadBuiltinRules()
{
    // Domain blocklist
    for (int i = 0; kBuiltinDomains[i] != nullptr; ++i) {
        QString domain = QString::fromUtf8(kBuiltinDomains[i]).toLower().trimmed();
        if (domain.isEmpty()) continue;
        m_blockedBaseDomains.insert(domain);
    }

    // URL pattern rules
    for (int i = 0; kBuiltinUrlRules[i] != nullptr; ++i) {
        FilterRule rule = parseRule(QString::fromUtf8(kBuiltinUrlRules[i]));
        if (rule.type != FilterRule::Type::Exception)
            m_blockRules.append(std::move(rule));
    }

    qDebug() << "[AdBlocker] Loaded"
             << m_blockedBaseDomains.size() << "domain rules +"
             << m_blockRules.size() << "URL pattern rules";
}

/* ─── Load external filter list (EasyList format) ─────────────────────── */
void AdBlocker::loadFilterList(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&f);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(u'!') || line.startsWith(u'['))
            continue;
        addRule(line);
    }
    qDebug() << "[AdBlocker] Loaded external list:" << filePath;
}

/* ─── Add a single rule (public API) ─────────────────────────────────── */
void AdBlocker::addRule(const QString &rawRule)
{
    FilterRule rule = parseRule(rawRule);
    if (rule.type == FilterRule::Type::Exception)
        m_exceptionRules.append(std::move(rule));
    else if (rule.type == FilterRule::Type::DomainExact)
        m_blockedDomains.insert(rule.pattern);
    else if (rule.type == FilterRule::Type::DomainSuffix)
        m_blockedBaseDomains.insert(rule.pattern);
    else
        m_blockRules.append(std::move(rule));
}

/* ─── Rule Parser ─────────────────────────────────────────────────────── */
FilterRule AdBlocker::parseRule(const QString &line) const
{
    FilterRule rule;
    QString text = line.trimmed();

    // Exception rules (whitelist)
    if (text.startsWith(QStringLiteral("@@"))) {
        rule.type = FilterRule::Type::Exception;
        text = text.mid(2);
        rule.regex = buildRegex(text);
        return rule;
    }

    // Extract options after $
    QString options;
    int dollarIdx = text.lastIndexOf(u'$');
    if (dollarIdx != -1) {
        options = text.mid(dollarIdx + 1).toLower();
        text = text.left(dollarIdx);
    }

    // Third-party only?
    if (options.contains(QStringLiteral("third-party")))
        rule.thirdPartyOnly = true;

    // Pure domain rule: ||example.com^
    if (text.startsWith(QStringLiteral("||")) && text.endsWith(u'^') &&
        !text.contains(u'*') && !text.contains(u'/'))
    {
        QString domain = text.mid(2, text.length() - 3).toLower();
        rule.type    = FilterRule::Type::DomainSuffix;
        rule.pattern = domain;
        return rule;
    }

    // URL pattern rule (|| prefix or wildcards)
    rule.type  = FilterRule::Type::UrlPattern;
    rule.regex = buildRegex(text);
    return rule;
}

/* ─── Regex builder from EasyList pattern syntax ──────────────────────── */
QRegularExpression AdBlocker::buildRegex(QString pattern)
{
    // Strip leading ||
    bool anchored = false;
    if (pattern.startsWith(QStringLiteral("||"))) {
        anchored = true;
        pattern  = pattern.mid(2);
    } else if (pattern.startsWith(u'|')) {
        pattern = u'^' + pattern.mid(1);
    }

    QString escaped;
    escaped.reserve(pattern.size() * 2);
    for (const QChar ch : pattern) {
        switch (ch.unicode()) {
        case '*':  escaped += QStringLiteral(".*");          break;
        case '^':  escaped += QStringLiteral("(?:[/:?#&=]|$)"); break;
        case '.': case '+': case '?': case '{': case '}':
        case '(': case ')': case '[': case ']': case '\\':
        case '$': case '|':
            escaped += u'\\';
            escaped += ch;
            break;
        default:
            escaped += ch;
        }
    }

    if (anchored)
        escaped = QStringLiteral("(?:https?://|wss?://)[^/]*\\.?") + escaped;

    return QRegularExpression(escaped,
        QRegularExpression::CaseInsensitiveOption |
        QRegularExpression::DontCaptureOption);
}

/* ─── Hostname extraction ─────────────────────────────────────────────── */
QString AdBlocker::extractHostname(const QUrl &url)
{
    return url.host().toLower().trimmed();
}

QString AdBlocker::baseDomain(const QString &host)
{
    // Extract registered domain (last two labels)
    QStringList parts = host.split(u'.');
    if (parts.size() < 2) return host;
    return parts.at(parts.size() - 2) + u'.' + parts.last();
}

/* ─── Core block logic ────────────────────────────────────────────────── */
bool AdBlocker::isBlocked(const QUrl &url,
                           const QUrl &firstPartyUrl,
                           QWebEngineUrlRequestInfo::ResourceType /*type*/) const
{
    const QString host = extractHostname(url);
    if (host.isEmpty()) return false;

    // Third-party detection
    const QString fpHost    = extractHostname(firstPartyUrl);
    const bool    isThirdPt = !fpHost.isEmpty() && baseDomain(host) != baseDomain(fpHost);

    // 1. Exact domain match
    if (m_blockedDomains.contains(host)) return true;

    // 2. Suffix / base domain match — walk up the hostname tree
    {
        QString h = host;
        while (true) {
            if (m_blockedBaseDomains.contains(h)) return true;
            int dot = h.indexOf(u'.');
            if (dot == -1) break;
            h = h.mid(dot + 1);
        }
    }

    // 3. URL pattern rules
    const QString urlStr = url.toString();
    for (const FilterRule &rule : m_blockRules) {
        if (rule.thirdPartyOnly && !isThirdPt) continue;
        if (rule.type == FilterRule::Type::UrlPattern) {
            if (rule.regex.isValid() && rule.regex.match(urlStr).hasMatch())
                return true;
        }
    }

    return false;
}

/* ─── The interceptor — called on every network request ──────────────── */
void AdBlocker::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    m_requests.fetchAndAddRelaxed(1);

    if (!m_enabled) return;

    const QUrl url          = info.requestUrl();
    const QUrl firstParty   = info.firstPartyUrl();

    // Always enforce English locale in HTTP headers for all outgoing requests
    info.setHttpHeader(QByteArrayLiteral("Accept-Language"), QByteArrayLiteral("en-US,en;q=0.9"));

    // Never block the home page, qrc:// resources, or data: URIs
    const QString scheme = url.scheme();
    if (scheme == QStringLiteral("qrc") ||
        scheme == QStringLiteral("data") ||
        scheme == QStringLiteral("file"))
        return;

    // Check exception rules first
    const QString urlStr = url.toString();
    for (const FilterRule &ex : m_exceptionRules) {
        if (ex.regex.isValid() && ex.regex.match(urlStr).hasMatch())
            return; // whitelisted
    }

    if (isBlocked(url, firstParty, info.resourceType())) {
        info.block(true);
        m_blocked.fetchAndAddRelaxed(1);
    }
}

/* ─── Content Script Injection for YouTube, Spotify & Web Ads ─────────── */
QString AdBlocker::contentScriptSource() const
{
    static QString cachedSource;
    if (cachedSource.isEmpty()) {
        QFile f(QStringLiteral(":/titan_shield_content.js"));
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            cachedSource = QString::fromUtf8(f.readAll());
        }
    }
    return cachedSource;
}

void AdBlocker::installContentScript(QWebEngineProfile *profile)
{
    if (!profile) return;
    const QString src = contentScriptSource();
    if (src.isEmpty()) return;

    QWebEngineScript script;
    script.setName(QStringLiteral("TitanShieldContentEngine"));
    script.setSourceCode(src);
    script.setInjectionPoint(QWebEngineScript::DocumentCreation);
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setRunsOnSubFrames(true);

    profile->scripts()->insert(script);
    qDebug() << "[AdBlocker] Installed TitanShield content script into WebEngine profile";
}

void AdBlocker::injectContentScriptIntoView(QWebEngineView *view)
{
    if (!view || !view->page()) return;
    const QString src = contentScriptSource();
    if (!src.isEmpty()) {
        view->page()->runJavaScript(src);
    }
}

