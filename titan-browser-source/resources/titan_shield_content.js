/**
 * TitanShield Advanced Content Engine — Next-Gen Ad Purging & Auto-Skip
 * Operates across YouTube, Spotify, and the entire web.
 * Injected at DocumentCreation (MainWorld) + refreshed on navigation.
 */
(function () {
    'use strict';

    /* ─── 1. Neutralize Anti-Adblock Traps & Globals ──────────────────────── */
    try {
        window.canRunAds = true;
        window.isAdBlockActive = false;
        window.google_ad_status = 1;
        window._adblock = false;
        window.isAdblockActive = false;
    } catch (e) { }

    /* ─── 2. High-Priority Universal Cosmetic Purge Stylesheet ────────────── */
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

    /* ─── 3. Bulletproof YouTube Video Ad Destroyer ───────────────────────── */
    let ytAdMuted = false;
    let ytLastVolume = 1.0;

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
            // 1. Mute ad immediately
            if (video && !ytAdMuted) {
                ytLastVolume = video.volume;
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

    /* ─── 4. Spotify Web Player Auto-Skip & Audio Muter ───────────────────── */
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

    /* ─── 5. Main Execution Loop & Mutation Observer ──────────────────────── */
    function tick() {
        purgeYouTubeAds();
        purgeSpotifyAds();
    }

    // High frequency interval (30ms) for instantaneous ad skipping
    setInterval(tick, 30);

    // Mutation observer to capture dynamic DOM injections instantly
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
