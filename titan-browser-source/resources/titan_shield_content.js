/**
 * TitanShield Content Engine v3 — Safe Ad Skipper
 * 
 * Strategy:
 *   YouTube/YTM: Network requests are NOT blocked (bypass in adblocker.cpp).
 *                Ads are handled here via mute → skip → restore.
 *                Only the 'ad-showing' player class is used as the ad signal
 *                (all other DOM selectors have false positives).
 *   Spotify:     Cosmetic hiding + skip-forward on ad tracks.
 *   General:     CSS cosmetic hiding of common ad elements.
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

        Object.defineProperty(navigator, 'language', { get: () => 'en-US', configurable: true });
        Object.defineProperty(navigator, 'languages', { get: () => ['en-US', 'en'], configurable: true });

        if (window.location.hostname.includes('youtube.com')) {
            if (!document.cookie.includes('hl=en')) {
                document.cookie = "PREF=f6=40000000&hl=en&gl=US;domain=.youtube.com;path=/;max-age=31536000;SameSite=Lax";
            }
        }
        if (window.location.hostname.includes('google.')) {
            if (!document.cookie.includes('hl=en')) {
                document.cookie = "PREF=hl=en&gl=US;domain=" + window.location.hostname + ";path=/;max-age=31536000";
            }
        }
    } catch (e) { }

    /* ─── 2. Cosmetic Ad Hiding (CSS only — no video interference) ────────── */
    const COSMETIC_CSS = `
        /* General web ads */
        ins.adsbygoogle,
        div[id^="google_ads_"],
        div[id^="div-gpt-ad"],
        iframe[src*="doubleclick"],
        iframe[src*="googlesyndication"],
        .taboola-container,
        .outbrain-container,
        .sponsored-content,
        .sponsored-post,
        div[data-ad-unit],
        div[data-ad-slot],

        /* YouTube sidebar / feed ads */
        #masthead-ad,
        ytd-ad-slot-renderer,
        ytd-rich-item-renderer:has(ytd-ad-slot-renderer),
        ytd-promoted-sparkles-web-renderer,
        ytd-display-ad-renderer,
        ytd-statement-banner-renderer,
        ytd-in-feed-ad-layout-renderer,
        ytd-banner-promo-renderer,
        ytd-promoted-video-renderer,
        ytd-action-companion-ad-renderer,
        ytd-engagement-panel-section-list-renderer[target-id="engagement-panel-ads"],
        #player-ads,
        ytd-compact-promoted-video-renderer,
        ytd-companion-slot-renderer,
        .ytd-merch-shelf-renderer,

        /* YouTube Music promos */
        ytmusic-mealbar-promo-renderer,
        ytmusic-notification-action-renderer,

        /* Spotify cosmetic ads */
        .Root__ad-banner,
        [data-testid="ad-banner"],
        [data-testid="topbar-ad-slot"],
        div[class*="AdBanner"],
        div[data-test-id="ad-leaderboard"] {
            display: none !important;
            visibility: hidden !important;
            height: 0 !important;
            width: 0 !important;
            max-height: 0 !important;
            margin: 0 !important;
            padding: 0 !important;
        }
    `;

    function ensureCosmeticStyles() {
        if (!document.getElementById('titan-shield-style')) {
            const s = document.createElement('style');
            s.id = 'titan-shield-style';
            s.textContent = COSMETIC_CSS;
            (document.head || document.documentElement).appendChild(s);
        }
    }
    ensureCosmeticStyles();
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', ensureCosmeticStyles);
    }

    /* ─── 3. YouTube / YouTube Music — In-Stream Ad Skipper ───────────────── */
    let ytAdState = false; // true = we muted an ad, need to restore later

    function handleYouTubeAds() {
        const host = window.location.hostname || '';
        if (!host.includes('youtube.com')) return;

        // ── Dismiss anti-adblock modals ──
        const modal = document.querySelector(
            'ytd-enforcement-message-view-model, ' +
            'tp-yt-paper-dialog.ytd-popup-container, ' +
            'yt-confirm-dialog-renderer'
        );
        if (modal) {
            try {
                const popup = modal.closest('.ytd-popup-container') ||
                              modal.closest('tp-yt-paper-dialog') || modal;
                popup.remove();
            } catch (e) { }
            document.querySelectorAll('tp-yt-iron-overlay-backdrop').forEach(el => {
                try { el.remove(); } catch (e) { }
            });
            if (document.body) document.body.style.overflow = 'auto';
        }

        // ── Dismiss YouTube Music promo banners ──
        document.querySelectorAll(
            'ytmusic-mealbar-promo-renderer, ytmusic-notification-action-renderer'
        ).forEach(el => { try { el.remove(); } catch (e) { } });

        // ── Detect in-stream ad — ONLY via player class (zero false positives) ──
        const player = document.getElementById('movie_player') ||
                       document.querySelector('.html5-video-player');
        if (!player) return;

        const adPlaying = player.classList.contains('ad-showing') ||
                          player.classList.contains('ad-interrupting');

        const video = document.querySelector('video.html5-main-video') ||
                      document.querySelector('video');

        if (adPlaying && video) {
            // Mute the ad audio
            if (!ytAdState) {
                video.muted = true;
                ytAdState = true;
            }

            // Fast-forward through the ad (only if duration is known)
            if (Number.isFinite(video.duration) && video.duration > 0.5) {
                video.playbackRate = 16.0;
                video.currentTime = video.duration - 0.1;
            }

            // Try native skip API
            if (typeof player.skipAd === 'function') {
                try { player.skipAd(); } catch (e) { }
            }

            // Click all known skip buttons
            const skipBtns = document.querySelectorAll(
                '.ytp-skip-ad-button, ' +
                '.ytp-ad-skip-button-modern, ' +
                '.ytp-ad-skip-button, ' +
                'button[id^="skip-button:"], ' +
                '.ytp-ad-skip-button-slot button, ' +
                '[aria-label="Skip ad"], ' +
                '[aria-label="Skip Ad"]'
            );
            skipBtns.forEach(btn => {
                try { btn.click(); } catch (e) { }
            });

        } else if (!adPlaying && ytAdState && video) {
            // ── Ad ended: restore normal playback ──
            video.muted = false;
            video.playbackRate = 1.0;
            ytAdState = false;

            // Resume playback if paused by ad transition
            if (video.paused) {
                try { video.play(); } catch (e) { }
            }
        }
    }

    /* ─── 4. Spotify — Ad Track Skipper ───────────────────────────────────── */
    let spotifyAdState = false;

    function isSpotifyAd() {
        if (document.querySelector('[aria-label="Advertisement"]')) return true;
        if (document.querySelector('[data-testid="ad-banner"]')) return true;
        if (document.querySelector('[data-testid="topbar-ad-slot"]')) return true;
        if (document.querySelector('[data-testid="now-playing-widget"] [href*="spotify:ad"]')) return true;

        const ctx = document.querySelector('[data-testid="context-item-link"]');
        if (ctx) {
            const t = (ctx.textContent || '').toLowerCase().trim();
            if (t === 'advertisement' || t === 'sponsored') return true;
        }

        const title = document.querySelector(
            '[data-testid="now-playing-widget"] [data-testid="track-info-name"]'
        );
        if (title) {
            const t = (title.textContent || '').toLowerCase().trim();
            if (t === 'advertisement') return true;
        }

        return false;
    }

    function handleSpotifyAds() {
        if (!window.location.hostname.includes('spotify.com')) return;

        const adActive = isSpotifyAd();
        const media = Array.from(document.querySelectorAll('audio, video'));

        if (adActive) {
            for (const m of media) {
                if (!spotifyAdState) {
                    m.muted = true;
                    spotifyAdState = true;
                }
                if (Number.isFinite(m.duration) && m.duration > 0) {
                    m.currentTime = m.duration - 0.05;
                }
            }
            const skip = document.querySelector(
                '[data-testid="control-button-skip-forward"], [aria-label="Next"]'
            );
            if (skip && !skip.disabled) {
                try { skip.click(); } catch (e) { }
            }
        } else if (!adActive && spotifyAdState) {
            for (const m of media) {
                m.muted = false;
                m.playbackRate = 1.0;
            }
            spotifyAdState = false;
        }
    }

    /* ─── 5. Main Loop ────────────────────────────────────────────────────── */
    function tick() {
        handleYouTubeAds();
        handleSpotifyAds();
    }

    // Run at moderate frequency — 200ms is plenty for skip buttons
    setInterval(tick, 200);

    // Also run on DOM mutations for instant response to ad injection
    const obs = new MutationObserver(tick);
    function startObserver() {
        if (document.body) {
            obs.observe(document.body, { childList: true, subtree: true });
        } else {
            setTimeout(startObserver, 200);
        }
    }
    startObserver();

})();
