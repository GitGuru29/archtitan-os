/**
 * TitanShield Advanced Content Engine — Ultra-Grade Ad Destroyer
 * Intercepts YouTube & Spotify player payloads (JSON/fetch/XHR), skips in-stream video ads,
 * neutralizes anti-adblock modals, and purges all cosmetic web advertising slots.
 */
(function () {
    'use strict';

    /* ─── 1. Neutralize Anti-Adblock Globals & Enforce English Locale ─────── */
    try {
        window.canRunAds = true;
        window.isAdBlockActive = false;
        window.google_ad_status = 1;
        window._adblock = false;
        window.isAdblockActive = false;

        // Force browser navigator language properties to English
        Object.defineProperty(navigator, 'language', { get: () => 'en-US', configurable: true });
        Object.defineProperty(navigator, 'languages', { get: () => ['en-US', 'en'], configurable: true });

        // Enforce English on YouTube
        if (window.location.hostname.includes('youtube.com')) {
            if (!document.cookie.includes('hl=en')) {
                document.cookie = "PREF=f6=40000000&hl=en&gl=US;domain=.youtube.com;path=/;max-age=31536000;SameSite=Lax";
            }
        }
        // Enforce English on Google
        if (window.location.hostname.includes('google.')) {
            if (!document.cookie.includes('hl=en')) {
                document.cookie = "PREF=hl=en&gl=US;domain=" + window.location.hostname + ";path=/;max-age=31536000";
            }
        }
    } catch (e) { }

    /* ─── 2. YouTube JSON & Network Payload Interceptor (Root-Level Ad Block) ─── */
    function sanitizeYTData(obj) {
        if (!obj || typeof obj !== 'object') return obj;

        try {
            // Strip ad placements, midrolls, prerolls, companion ads
            if (obj.adPlacements) delete obj.adPlacements;
            if (obj.adSlots) delete obj.adSlots;
            if (obj.playerAds) delete obj.playerAds;
            if (obj.adBreakHeartbeatParams) delete obj.adBreakHeartbeatParams;
            if (obj.paidContentOverlay) delete obj.paidContentOverlay;

            // Neutralize YouTube Anti-Adblock Enforcement modals
            if (obj.auxiliaryUi && obj.auxiliaryUi.messageRenderers) {
                if (obj.auxiliaryUi.messageRenderers.enforcementMessageViewModel) {
                    delete obj.auxiliaryUi.messageRenderers.enforcementMessageViewModel;
                }
            }
            if (obj.enforcementMessageViewModel) {
                delete obj.enforcementMessageViewModel;
            }

            // Recursively sanitize playerResponse
            if (obj.playerResponse && typeof obj.playerResponse === 'object') {
                sanitizeYTData(obj.playerResponse);
            }
        } catch (e) { }

        return obj;
    }

    // Intercept window.ytInitialPlayerResponse
    let rawPlayerResponse = window.ytInitialPlayerResponse;
    try {
        Object.defineProperty(window, 'ytInitialPlayerResponse', {
            get: function () {
                return rawPlayerResponse;
            },
            set: function (val) {
                rawPlayerResponse = sanitizeYTData(val);
            },
            configurable: true
        });
    } catch (e) { }

    // Intercept JSON.parse
    const origJSONParse = JSON.parse;
    JSON.parse = function (text, reviver) {
        let data = origJSONParse.apply(this, arguments);
        try {
            if (data && typeof data === 'object') {
                if (data.adPlacements || data.adSlots || data.playerAds || data.adBreakHeartbeatParams) {
                    sanitizeYTData(data);
                }
            }
        } catch (e) { }
        return data;
    };

    // Intercept window.fetch
    const origFetch = window.fetch;
    window.fetch = async function (...args) {
        const response = await origFetch.apply(this, args);
        try {
            const url = args[0] ? (typeof args[0] === 'string' ? args[0] : args[0].url || '') : '';
            if (url.includes('/youtubei/v1/player') || url.includes('/youtubei/v1/next') || url.includes('player_response')) {
                const clone = response.clone();
                const json = await clone.json();
                sanitizeYTData(json);
                return new Response(JSON.stringify(json), {
                    status: response.status,
                    statusText: response.statusText,
                    headers: response.headers
                });
            }
        } catch (e) { }
        return response;
    };

    // Intercept XMLHttpRequest
    const origXHROpen = XMLHttpRequest.prototype.open;
    const origXHRSend = XMLHttpRequest.prototype.send;
    XMLHttpRequest.prototype.open = function (method, url) {
        this._reqUrl = url;
        return origXHROpen.apply(this, arguments);
    };
    XMLHttpRequest.prototype.send = function (body) {
        if (this._reqUrl && typeof this._reqUrl === 'string' &&
            (this._reqUrl.includes('/youtubei/v1/player') || this._reqUrl.includes('/youtubei/v1/next'))) {
            this.addEventListener('readystatechange', function () {
                if (this.readyState === 4 && this.status === 200) {
                    try {
                        const parsed = origJSONParse(this.responseText);
                        sanitizeYTData(parsed);
                        const sanitizedStr = JSON.stringify(parsed);
                        Object.defineProperty(this, 'responseText', { value: sanitizedStr, writable: true });
                        Object.defineProperty(this, 'response', { value: sanitizedStr, writable: true });
                    } catch (e) { }
                }
            });
        }
        return origXHRSend.apply(this, arguments);
    };

    /* ─── 3. High-Priority Universal Cosmetic Purge Stylesheet ────────────── */
    const COSMETIC_CSS = `
        /* ── General Web Ads ── */
        ins.adsbygoogle,
        div[id^="google_ads_"],
        div[id^="div-gpt-ad"],
        div[class*="ad-container"],
        div[class*="ad_container"],
        div[class*="ad-slot"],
        div[class*="ad_slot"],
        div[id*="ad-slot"],
        div[id*="ad_slot"],
        iframe[src*="doubleclick"],
        iframe[src*="adservice"],
        iframe[src*="googlesyndication"],
        iframe[src*="pagead"],
        .taboola-container,
        .outbrain-container,
        .trc_related_container,
        .sponsored-content,
        .sponsored-post,
        .advertisement-box,
        .ad-banner,
        .top-ad-banner,
        div[data-ad-unit],
        div[data-ad-slot],

        /* ── YouTube Ad Elements ── */
        #masthead-ad,
        ytd-ad-slot-renderer,
        ytd-rich-item-renderer:has(ytd-ad-slot-renderer),
        ytd-promoted-sparkles-web-renderer,
        ytd-display-ad-renderer,
        ytd-statement-banner-renderer,
        ytd-in-feed-ad-layout-renderer,
        ytd-banner-promo-renderer,
        ytd-promoted-video-renderer,
        .ytd-merch-shelf-renderer,
        ytd-action-companion-ad-renderer,
        .ytp-ad-overlay-container,
        .ytp-ad-message-container,
        .ytp-ad-progress-list,
        ytd-engagement-panel-section-list-renderer[target-id="engagement-panel-ads"],
        #player-ads,
        #panels:has(ytd-ads-engagement-panel-content-renderer),
        ytd-compact-promoted-video-renderer,
        .ytd-player-legacy-desktop-watch-ads-renderer,
        ytd-companion-slot-renderer,
        ytd-in-feed-ad-layout-renderer,

        /* ── Spotify Ad Elements ── */
        .Root__ad-banner,
        [data-testid="ad-banner"],
        [aria-label="Advertisement"],
        .ad-iframe,
        div[data-test-id="ad-leaderboard"],
        .ad-placeholder,
        div[class*="AdBanner"],
        div[class*="sponsored"],
        div[data-testid="topbar-ad-slot"] {
            display: none !important;
            opacity: 0 !important;
            pointer-events: none !important;
            visibility: hidden !important;
            height: 0 !important;
            width: 0 !important;
            max-height: 0 !important;
            max-width: 0 !important;
            margin: 0 !important;
            padding: 0 !important;
        }
    `;

    function ensureCosmeticStyles() {
        if (!document.getElementById('titan-shield-style')) {
            const style = document.createElement('style');
            style.id = 'titan-shield-style';
            style.textContent = COSMETIC_CSS;
            const target = document.head || document.documentElement || document.body;
            if (target) target.appendChild(style);
        }
    }
    ensureCosmeticStyles();
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', ensureCosmeticStyles);
    }

    /* ─── 4. Bulletproof YouTube Video Ad Fast-Forwarder & Auto-Skipper ────── */
    let ytAdMuted = false;

    function purgeYouTubeAds() {
        const host = window.location.hostname || '';
        if (!host.includes('youtube.com')) return;

        ensureCosmeticStyles();

        // A. Anti-Adblock Modal Auto-Dismissal
        const modal = document.querySelector('ytd-enforcement-message-view-model, tp-yt-paper-dialog.ytd-popup-container');
        if (modal) {
            const popup = modal.closest('.ytd-popup-container') || modal;
            popup.remove();
            document.querySelectorAll('tp-yt-iron-overlay-backdrop').forEach(el => el.remove());
            document.body.style.overflow = 'auto';
            const vid = document.querySelector('video.html5-main-video') || document.querySelector('video');
            if (vid && vid.paused) {
                vid.play();
            }
        }

        // B. In-Stream Video Ad Detection
        const player = document.getElementById('movie_player') || document.querySelector('.html5-video-player');
        const video = document.querySelector('video.html5-main-video') || document.querySelector('video');

        const hasAdClass = player && (
            player.classList.contains('ad-showing') ||
            player.classList.contains('ad-interrupting')
        );

        const hasAdOverlay = document.querySelector('.ytp-ad-player-overlay') !== null ||
            document.querySelector('.ytp-ad-player-overlay-layout') !== null ||
            document.querySelector('.ytp-ad-text') !== null ||
            document.querySelector('.ytp-ad-preview-text') !== null ||
            document.querySelector('.ytp-skip-ad-button') !== null ||
            document.querySelector('.ytp-ad-skip-button-modern') !== null ||
            document.querySelector('.video-ads.ytp-ad-module')?.childElementCount > 0;

        const isAdActive = hasAdClass || hasAdOverlay;

        if (isAdActive) {
            // 1. Mute ad immediately so no sound leaks
            if (video && !ytAdMuted) {
                video.muted = true;
                ytAdMuted = true;
            }

            // 2. Fast forward video past ad
            if (video) {
                video.playbackRate = 16.0;
                if (Number.isFinite(video.duration) && video.duration > 0) {
                    video.currentTime = video.duration + 1.0;
                }
            }

            // 3. Native YouTube player API skip
            if (player && typeof player.skipAd === 'function') {
                try { player.skipAd(); } catch (e) { }
            }

            // 4. Force Click all Skip Button variations with realistic MouseEvents
            const skipSelectors = [
                '.ytp-skip-ad-button',
                '.ytp-ad-skip-button-modern',
                'button.ytp-skip-ad-button',
                'button.ytp-ad-skip-button-modern',
                '.ytp-ad-skip-button',
                'button[id^="skip-button:"]',
                '.ytp-ad-skip-button-slot button',
                'button.ytp-ad-skip-button-container',
                '.ytp-ad-overlay-close-button',
                '.ytp-ad-survey button',
                'button[id^="visit-advertiser"]',
                'div.ytp-ad-player-overlay-skip-or-preview button',
                '[class*="ytp-ad-skip"]',
                '[class*="ytp-skip-ad"]'
            ];

            for (const selector of skipSelectors) {
                const buttons = document.querySelectorAll(selector);
                buttons.forEach(btn => {
                    if (btn) {
                        try {
                            btn.click();
                            ['pointerdown', 'mousedown', 'pointerup', 'mouseup', 'click'].forEach(evt => {
                                btn.dispatchEvent(new MouseEvent(evt, { bubbles: true, cancelable: true, view: window }));
                            });
                        } catch (e) { }
                    }
                });
            }
        } else if (!isAdActive && ytAdMuted && video) {
            // Restore clean video playback
            video.muted = false;
            video.playbackRate = 1.0;
            ytAdMuted = false;
        }
    }

    /* ─── 5. Spotify Web Player Auto-Skip & Audio Muter ───────────────────── */
    let spotifyAdMuted = false;

    function purgeSpotifyAds() {
        const host = window.location.hostname || '';
        if (!host.includes('spotify.com')) return;

        ensureCosmeticStyles();

        const mediaElements = Array.from(document.querySelectorAll('audio, video'));
        const nowPlaying = document.querySelector('[data-testid="now-playing-widget"]');
        const trackText = nowPlaying ? (nowPlaying.innerText || '').toLowerCase() : '';

        const isAd = trackText.includes('advertisement') ||
            document.querySelector('[aria-label="Advertisement"]') !== null ||
            document.querySelector('[data-testid="ad-banner"]') !== null;

        for (const media of mediaElements) {
            if (isAd || (media.duration < 35 && media.src.includes('audio-fa.scdn.co'))) {
                if (!spotifyAdMuted) {
                    media.muted = true;
                    spotifyAdMuted = true;
                }
                media.playbackRate = 16.0;
                if (Number.isFinite(media.duration) && media.duration > 0) {
                    media.currentTime = media.duration;
                }

                const skipBtn = document.querySelector('[data-testid="control-button-skip-forward"]');
                if (skipBtn && !skipBtn.disabled && typeof skipBtn.click === 'function') {
                    skipBtn.click();
                }
            } else if (!isAd && spotifyAdMuted) {
                media.muted = false;
                media.playbackRate = 1.0;
                spotifyAdMuted = false;
            }
        }
    }

    /* ─── 6. High-Frequency Tick & Mutation Observer ──────────────────────── */
    function tick() {
        purgeYouTubeAds();
        purgeSpotifyAds();
    }

    // High frequency interval (25ms) for instantaneous ad skipping
    setInterval(tick, 25);

    // Mutation observer to capture dynamic DOM injections instantly
    const domObserver = new MutationObserver(tick);
    function attachObserver() {
        if (document.body) {
            domObserver.observe(document.body, { childList: true, subtree: true, attributes: true });
        } else {
            setTimeout(attachObserver, 80);
        }
    }
    attachObserver();

})();
