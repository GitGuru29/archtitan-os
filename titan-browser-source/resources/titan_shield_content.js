/**
 * TitanShield Advanced Content Engine
 * Injected at DocumentCreation to eliminate video/audio ads on YouTube & Spotify,
 * remove anti-adblock nag modals, and cosmetically purge web advertising slots.
 */
(function () {
    'use strict';

    const hostname = window.location.hostname || '';
    const isYouTube = hostname.includes('youtube.com');
    const isSpotify = hostname.includes('spotify.com');

    /* ─── 1. Universal Anti-Adblock Trap Neutralizer ───────────────────────── */
    try {
        window.canRunAds = true;
        window.isAdBlockActive = false;
        window.google_ad_status = 1;
        window._adblock = false;
    } catch (e) { }

    /* ─── 2. Inject Universal Cosmetic Stylesheet ─────────────────────────── */
    function injectCosmeticStyles() {
        const cssRules = `
            /* Universal Ad Selectors */
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
            .top-ad-banner {
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

            /* YouTube Specific Element Hiding */
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
            ytd-compact-promoted-video-renderer {
                display: none !important;
                opacity: 0 !important;
                pointer-events: none !important;
                visibility: hidden !important;
                height: 0 !important;
                margin: 0 !important;
                padding: 0 !important;
            }

            /* Spotify Specific Element Hiding */
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
                margin: 0 !important;
                padding: 0 !important;
            }
        `;

        function applyCSS() {
            if (document.getElementById('titan-shield-style')) return;
            const style = document.createElement('style');
            style.id = 'titan-shield-style';
            style.textContent = cssRules;
            (document.head || document.documentElement).appendChild(style);
        }

        if (document.head || document.documentElement) {
            applyCSS();
        } else {
            document.addEventListener('DOMContentLoaded', applyCSS, { once: true });
        }
    }
    injectCosmeticStyles();

    /* ─── 3. YouTube Video Ad Bypass & Auto-Skipper ────────────────────────── */
    if (isYouTube) {
        let isAdMuted = false;
        let lastUserVolume = 1.0;

        function handleYouTubeAds() {
            // A. Anti-Adblock modal dismissal ("Ad blockers violate YouTube Terms")
            const enforcementDialog = document.querySelector('ytd-enforcement-message-view-model, tp-yt-paper-dialog.ytd-popup-container');
            if (enforcementDialog) {
                const popupContainer = enforcementDialog.closest('.ytd-popup-container') || enforcementDialog;
                popupContainer.remove();
                document.querySelectorAll('tp-yt-iron-overlay-backdrop').forEach(el => el.remove());
                document.body.style.overflow = 'auto';
                const mainVideo = document.querySelector('video.html5-main-video') || document.querySelector('video');
                if (mainVideo && mainVideo.paused) {
                    mainVideo.play();
                }
            }

            // B. Detect player ad state
            const player = document.querySelector('#movie_player') || document.querySelector('.html5-video-player');
            const video = document.querySelector('video.html5-main-video') || document.querySelector('video');
            const adShowing = player && (
                player.classList.contains('ad-showing') ||
                player.classList.contains('ad-interrupting') ||
                document.querySelector('.ytp-ad-player-overlay') !== null ||
                document.querySelector('.ytp-ad-text') !== null
            );

            if (adShowing && video) {
                if (!isAdMuted) {
                    lastUserVolume = video.volume;
                    video.muted = true;
                    isAdMuted = true;
                }

                // Super-speed fast forward the ad
                video.playbackRate = 16.0;

                // Jump directly to the end of the ad segment
                if (Number.isFinite(video.duration) && video.duration > 0) {
                    video.currentTime = video.duration + 0.5;
                }

                // Instant click on any visible skip button
                const skipSelectors = [
                    '.ytp-ad-skip-button',
                    '.ytp-ad-skip-button-modern',
                    '.ytp-skip-ad-button',
                    '.ytp-ad-skip-button-slot button',
                    'button.ytp-ad-skip-button-container',
                    '.ytp-ad-overlay-close-button',
                    '.ytp-ad-survey button',
                    'button[id^="visit-advertiser"]'
                ];

                for (const selector of skipSelectors) {
                    const btn = document.querySelector(selector);
                    if (btn && typeof btn.click === 'function') {
                        btn.click();
                        break;
                    }
                }
            } else if (!adShowing && isAdMuted && video) {
                // Restore normal user volume and playback speed
                video.muted = false;
                video.playbackRate = 1.0;
                isAdMuted = false;
            }
        }

        // Run on high-frequency interval for immediate ad skipping
        setInterval(handleYouTubeAds, 50);

        // Also observe DOM changes in the player container
        const observer = new MutationObserver(handleYouTubeAds);
        function initYouTubeObserver() {
            const playerEl = document.querySelector('#movie_player') || document.body;
            if (playerEl) {
                observer.observe(playerEl, { childList: true, subtree: true, attributes: true });
            } else {
                setTimeout(initYouTubeObserver, 200);
            }
        }
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', initYouTubeObserver);
        } else {
            initYouTubeObserver();
        }
    }

    /* ─── 4. Spotify Web Player Ad Bypass & Auto-Skipper ───────────────────── */
    if (isSpotify) {
        let spotifyAdMuted = false;

        function handleSpotifyAds() {
            // Find Spotify media playback element
            const mediaElements = Array.from(document.querySelectorAll('audio, video'));

            // Check if current track is an advertisement
            const nowPlayingWidget = document.querySelector('[data-testid="now-playing-widget"]');
            const trackText = nowPlayingWidget ? nowPlayingWidget.innerText || '' : '';
            const isAdTrack = trackText.toLowerCase().includes('advertisement') ||
                document.querySelector('[aria-label="Advertisement"]') !== null ||
                document.querySelector('[data-testid="ad-banner"]') !== null;

            for (const media of mediaElements) {
                if (isAdTrack || media.duration < 35 && media.src.includes('audio-fa.scdn.co')) {
                    if (!spotifyAdMuted) {
                        media.muted = true;
                        spotifyAdMuted = true;
                    }
                    media.playbackRate = 16.0;
                    if (Number.isFinite(media.duration) && media.duration > 0) {
                        media.currentTime = media.duration;
                    }

                    // Click Spotify skip forward control
                    const skipBtn = document.querySelector('[data-testid="control-button-skip-forward"]');
                    if (skipBtn && !skipBtn.disabled && typeof skipBtn.click === 'function') {
                        skipBtn.click();
                    }
                } else if (!isAdTrack && spotifyAdMuted) {
                    media.muted = false;
                    media.playbackRate = 1.0;
                    spotifyAdMuted = false;
                }
            }
        }

        setInterval(handleSpotifyAds, 100);
    }
})();
