/**
 * TitanShield Advanced Content Engine — Ultra-Clean Ad Destroyer & Media Protector
 * Eliminates YouTube/YTM & Spotify in-stream ads, dismisses anti-adblock modals,
 * and hides cosmetic ad banners without interfering with legitimate audio/video streams.
 */
(function () {
    'use strict';

    /* ─── 1. Anti-Adblock Defusal & English Locale ───────────────────────── */
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

    /* ─── 2. Universal Cosmetic Ad Purge Stylesheet ───────────────────────── */
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
        ytd-engagement-panel-section-list-renderer[target-id="engagement-panel-ads"],
        #player-ads,
        #panels:has(ytd-ads-engagement-panel-content-renderer),
        ytd-compact-promoted-video-renderer,
        .ytd-player-legacy-desktop-watch-ads-renderer,
        ytd-companion-slot-renderer,

        /* ── YouTube Music Promo Elements ── */
        ytmusic-mealbar-promo-renderer,
        ytmusic-notification-action-renderer,

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

    /* ─── 3. YouTube & YouTube Music — Video Ad Auto-Skipper ──────────────── */
    let ytAdMuted = false;

    function purgeYouTubeAds() {
        const host = window.location.hostname || '';
        if (!host.includes('youtube.com')) return;

        ensureCosmeticStyles();

        // 1. Auto-dismiss anti-adblock modals
        const modal = document.querySelector('ytd-enforcement-message-view-model, tp-yt-paper-dialog.ytd-popup-container, yt-confirm-dialog-renderer');
        if (modal) {
            const popup = modal.closest('.ytd-popup-container') || modal.closest('tp-yt-paper-dialog') || modal;
            popup.remove();
            document.querySelectorAll('tp-yt-iron-overlay-backdrop').forEach(el => el.remove());
            if (document.body) document.body.style.overflow = 'auto';
            const vid = document.querySelector('video.html5-main-video') || document.querySelector('video');
            if (vid && vid.paused) {
                vid.play();
            }
        }

        // 2. Detect in-stream video ads
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
            // Mute ad
            if (video && !ytAdMuted) {
                video.muted = true;
                ytAdMuted = true;
            }

            // Fast-forward to end of ad and force ended event
            if (video) {
                video.playbackRate = 16.0;
                if (Number.isFinite(video.duration) && video.duration > 0) {
                    video.currentTime = video.duration;
                } else {
                    video.currentTime = 99999;
                }
                if (video.paused) {
                    try { video.play(); } catch (e) { }
                }
                try {
                    video.dispatchEvent(new Event('timeupdate', { bubbles: true }));
                    video.dispatchEvent(new Event('ended', { bubbles: true }));
                } catch (e) { }
            }

            // Native skip API
            if (player && typeof player.skipAd === 'function') {
                try { player.skipAd(); } catch (e) { }
            }

            // Click skip buttons
            const skipSelectors = [
                '.ytp-skip-ad-button',
                '.ytp-ad-skip-button-modern',
                'button.ytp-skip-ad-button',
                'button.ytp-ad-skip-button-modern',
                '.ytp-ad-skip-button',
                'button[id^="skip-button:"]',
                '.ytp-ad-skip-button-slot button',
                '.ytp-ad-overlay-close-button',
                'div.ytp-ad-player-overlay-skip-or-preview button',
                '[class*="ytp-ad-skip"]',
                '[class*="ytp-skip-ad"]',
                '[aria-label="Skip ad"]'
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
            // Unmute and restore 1.0x playback speed for clean music/video
            video.muted = false;
            video.playbackRate = 1.0;
            ytAdMuted = false;
        }
    }

    /* ─── 4. Spotify Web Player — Ad Skipper ──────────────────────────────── */
    let spotifyAdMuted = false;

    function isSpotifyAdPlaying() {
        if (document.querySelector('[aria-label="Advertisement"]')) return true;
        if (document.querySelector('[data-testid="ad-banner"]')) return true;
        if (document.querySelector('[data-testid="topbar-ad-slot"]')) return true;
        if (document.querySelector('[data-testid="now-playing-widget"] [data-testid="context-item-link"][href*="spotify:ad"]')) return true;

        const npContext = document.querySelector('[data-testid="context-item-link"]');
        if (npContext) {
            const txt = (npContext.textContent || '').toLowerCase();
            if (txt === 'advertisement' || txt === 'sponsored') return true;
        }

        const entityTitle = document.querySelector('[data-testid="now-playing-widget"] [data-testid="track-info-name"]');
        if (entityTitle) {
            const t = (entityTitle.textContent || '').toLowerCase();
            if (t === 'advertisement' || t === 'spotify') return true;
        }

        return false;
    }

    function purgeSpotifyAds() {
        const host = window.location.hostname || '';
        if (!host.includes('spotify.com')) return;

        ensureCosmeticStyles();

        const adActive = isSpotifyAdPlaying();
        const mediaElements = Array.from(document.querySelectorAll('audio, video'));

        if (adActive) {
            for (const media of mediaElements) {
                if (!spotifyAdMuted) {
                    media.muted = true;
                    spotifyAdMuted = true;
                }
                if (Number.isFinite(media.duration) && media.duration > 0) {
                    media.currentTime = media.duration - 0.05;
                }
            }

            const skipBtn = document.querySelector('[data-testid="control-button-skip-forward"]') ||
                document.querySelector('[aria-label="Next"]');
            if (skipBtn && !skipBtn.disabled && typeof skipBtn.click === 'function') {
                skipBtn.click();
            }
        } else if (!adActive && spotifyAdMuted) {
            for (const media of mediaElements) {
                media.muted = false;
                media.playbackRate = 1.0;
                spotifyAdMuted = false;
            }
        }
    }

    /* ─── 5. Periodic Tick & Observer ─────────────────────────────────────── */
    function tick() {
        purgeYouTubeAds();
        purgeSpotifyAds();
    }

    setInterval(tick, 50);

    const domObserver = new MutationObserver(tick);
    function attachObserver() {
        if (document.body) {
            domObserver.observe(document.body, { childList: true, subtree: true, attributes: true });
        } else {
            setTimeout(attachObserver, 100);
        }
    }
    attachObserver();

})();
