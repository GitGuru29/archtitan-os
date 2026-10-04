-- ArchTitan Default Hyprland Config (Lua)
-- Requires Hyprland >= 0.55 (Lua config support).
-- If this file exists, Hyprland ignores hyprland.conf entirely.

-------------------
---- MONITORS ----
-------------------

hl.monitor({
    output   = "",
    mode     = "preferred",
    position = "auto",
    scale    = "1",
})

---------------------
---- MY PROGRAMS ----
---------------------

local terminal    = "kitty"
local menu        = "rofi -show drun"
local browser     = "titanbrowser"
local fileManager = terminal .. " -e ranger"

-- Sandbox exec wrapper
-- Routes all user app launches through titan-sandboxd policy resolution.
-- System daemons (autostart) are intentionally excluded -- they must not be boxed.
local sandboxed = "titan-exec-hook"

-----------------------
---- ENVIRONMENT ----
-----------------------

hl.env("XCURSOR_SIZE", "24")
hl.env("QT_QPA_PLATFORMTHEME", "qt5ct")
hl.env("WLR_NO_HARDWARE_CURSORS", "1")
hl.env("WLR_RENDERER_ALLOW_SOFTWARE", "1")
hl.env("AQ_NO_MODIFIERS", "1")

-------------------
---- AUTOSTART ----
-------------------

hl.on("hyprland.start", function()
    hl.exec_cmd("bash -c 'sleep 2 && waybar'")
    hl.exec_cmd("bash -c 'sleep 3 && titan-media-hud'")
    hl.exec_cmd("titan-wallpaper-restore")
    hl.exec_cmd("bash -c 'sleep 3 && titan-quicksettings --daemon'")
    hl.exec_cmd("/usr/lib/polkit-kde-authentication-agent-1")
    hl.exec_cmd("xdg-user-dirs-update")

    -- Auto-launch installer as floating window (only on live ISO)
    -- /run/archiso only exists on mkarchiso live media -- reliable detection
    -- Users can minimize it and explore the desktop, then reopen with Super+I
    hl.exec_cmd("bash -c 'sleep 6; if [ -d /run/archiso ]; then launch-installer; fi'")

    -- Disable screen timeout / DPMS blanking on live ISO
    -- No hypridle daemon is launched -- no idle daemon = no blanking
    -- Also explicitly kill any DPMS via wlopm on startup
    hl.exec_cmd("bash -c 'sleep 2; which wlopm >/dev/null 2>&1 && wlopm --on DP-1 --on HDMI-A-1 --on Virtual-1 || true'")
end)

---------------------------
---- KEYBINDINGS ----
---------------------------

local mainMod = "SUPER"

-- Shell & Utilities
hl.bind(mainMod .. " + SUPER_L", hl.dsp.exec_cmd(sandboxed .. " " .. menu), { repeating = true })  -- Windows logo to open search
hl.bind(mainMod .. " + Space", hl.dsp.exec_cmd(sandboxed .. " " .. menu))   -- Spotlight-style: Super+Space
hl.bind(mainMod .. " + Tab", hl.dsp.exec_cmd(sandboxed .. " " .. menu))     -- Toggle overview/search
hl.bind(mainMod .. " + V", hl.dsp.exec_cmd("cliphist list | rofi -dmenu | cliphist decode | wl-copy"))  -- Clipboard
hl.bind(mainMod .. " + Period", hl.dsp.exec_cmd(sandboxed .. " rofi -show emoji"))  -- Emoji
hl.bind(mainMod .. " + backslash", hl.dsp.exec_cmd("titan-wallpaper-picker"))  -- Wallpaper Picker (Super+\)
hl.bind(mainMod .. " + SHIFT + backslash", hl.dsp.exec_cmd("titan-wallpaper-picker"))  -- Wallpaper Picker (Super+|)
hl.bind(mainMod .. " + SHIFT + S", hl.dsp.exec_cmd('grim -g "$(slurp)" - | wl-copy'))  -- Screen snip
hl.bind("Print", hl.dsp.exec_cmd('grim -g "$(slurp)" - | wl-copy'))
hl.bind(mainMod .. " + N", hl.dsp.exec_cmd("titan-quicksettings --toggle"))  -- Quick Settings & Notification Center

-- Session & Media
hl.bind("CTRL + ALT + Delete", hl.dsp.exec_cmd("titan-powermenu"))
-- Disable swaylock on live ISO so you don't get locked out without a password
hl.bind(mainMod .. " + L", hl.dsp.exec_cmd(":"))  -- Lock disabled on live OS
hl.bind(mainMod .. " + SHIFT + L", hl.dsp.exec_cmd("systemctl suspend"))  -- Sleep
hl.bind("XF86Sleep", hl.dsp.exec_cmd(":"))  -- swallow sleep key on live ISO

-- Apps
hl.bind(mainMod .. " + Return", hl.dsp.exec_cmd(sandboxed .. " " .. terminal))        -- Terminal
hl.bind(mainMod .. " + E", hl.dsp.exec_cmd(sandboxed .. " " .. fileManager))          -- File manager
hl.bind(mainMod .. " + W", hl.dsp.exec_cmd(sandboxed .. " " .. browser))              -- Browser
hl.bind(mainMod .. " + C", hl.dsp.exec_cmd(sandboxed .. " code"))                     -- Code editor
hl.bind(mainMod .. " + X", hl.dsp.exec_cmd(sandboxed .. " " .. terminal .. " -e nano"))  -- Text editor
hl.bind("CTRL + " .. mainMod .. " + V", hl.dsp.exec_cmd(sandboxed .. " pavucontrol"))  -- Volume mixer
hl.bind("CTRL + SHIFT + Escape", hl.dsp.exec_cmd(sandboxed .. " " .. terminal .. " -e btop"))  -- Task manager
hl.bind(mainMod .. " + comma", hl.dsp.exec_cmd(sandboxed .. " archtitan-settings"))     -- Settings panel shortcut
hl.bind(mainMod .. " + I", hl.dsp.exec_cmd("bash -c 'if [ -d /run/archiso ]; then launch-installer; else titan-exec-hook archtitan-settings; fi'"))  -- Installer on Live ISO / Settings on Installed OS

-- Window Management
hl.bind(mainMod .. " + Q", hl.dsp.window.close())                              -- Close
hl.bind(mainMod .. " + SHIFT + ALT + Q", hl.dsp.exec_cmd("hyprctl kill"))      -- Forcefully zap a window
hl.bind(mainMod .. " + ALT + Space", hl.dsp.window.float({ action = "toggle" }))  -- Float/Tile
hl.bind(mainMod .. " + F", hl.dsp.window.fullscreen({ mode = "fullscreen" }))    -- Fullscreen
hl.bind(mainMod .. " + D", hl.dsp.window.fullscreen({ mode = "maximized" }))     -- Maximize (fullscreen with gaps)
hl.bind(mainMod .. " + P", hl.dsp.window.pseudo())                              -- dwindle
hl.bind(mainMod .. " + J", hl.dsp.layout("togglesplit"))                        -- Adjust split ratio (toggle)

-- Move Focus
hl.bind(mainMod .. " + left",  hl.dsp.focus({ direction = "left" }))
hl.bind(mainMod .. " + right", hl.dsp.focus({ direction = "right" }))
hl.bind(mainMod .. " + up",    hl.dsp.focus({ direction = "up" }))
hl.bind(mainMod .. " + down",  hl.dsp.focus({ direction = "down" }))

-- Move Window
hl.bind(mainMod .. " + SHIFT + left",  hl.dsp.window.move({ direction = "left" }))
hl.bind(mainMod .. " + SHIFT + right", hl.dsp.window.move({ direction = "right" }))
hl.bind(mainMod .. " + SHIFT + up",    hl.dsp.window.move({ direction = "up" }))
hl.bind(mainMod .. " + SHIFT + down",  hl.dsp.window.move({ direction = "down" }))

-- Workspaces
for i = 1, 9 do
    hl.bind(mainMod .. " + " .. i, hl.dsp.focus({ workspace = i }))
    hl.bind(mainMod .. " + SHIFT + " .. i, hl.dsp.window.move({ workspace = i }))
end

-- Focus Workspace Left/Right
hl.bind("CTRL + " .. mainMod .. " + left",  hl.dsp.focus({ workspace = "e-1" }))
hl.bind("CTRL + " .. mainMod .. " + right", hl.dsp.focus({ workspace = "e+1" }))
hl.bind(mainMod .. " + Page_Down", hl.dsp.focus({ workspace = "e+1" }))
hl.bind(mainMod .. " + Page_Up",   hl.dsp.focus({ workspace = "e-1" }))
hl.bind(mainMod .. " + mouse_down", hl.dsp.focus({ workspace = "e+1" }))
hl.bind(mainMod .. " + mouse_up",   hl.dsp.focus({ workspace = "e-1" }))

-- Scratchpad
hl.bind(mainMod .. " + S", hl.dsp.workspace.toggle_special("magic"))
hl.bind(mainMod .. " + ALT + S", hl.dsp.window.move({ workspace = "special:magic" }))

-- Mouse bindings
hl.bind(mainMod .. " + mouse:272", hl.dsp.window.drag(), { mouse = true })
hl.bind(mainMod .. " + mouse:273", hl.dsp.window.resize(), { mouse = true })

-----------------------
---- LOOK AND FEEL ----
-----------------------

hl.config({
    input = {
        kb_layout   = "us",
        follow_mouse = 1,
        sensitivity = 0,
        touchpad = {
            natural_scroll = true,
            tap_to_click  = true,
        },
    },

    cursor = {
        no_hardware_cursors = true,
    },

    general = {
        gaps_in    = 5,
        gaps_out   = 10,
        border_size = 2,
        col = {
            active_border   = { colors = { "rgba(7aa2f7ee)", "rgba(9ece6aee)", "rgba(e0af68ee)", "rgba(bb9af7ee)" }, angle = 45 },
            inactive_border = "rgba(414868aa)",
        },
        layout = "dwindle",
    },

    -- Decoration -- blur, shadows, rounding
    decoration = {
        rounding = 12,

        blur = {
            enabled = false,
        },

        shadow = {
            enabled      = true,
            range        = 12,
            render_power = 3,
            offset       = { 0, 4 },
            color        = "rgba(1a1a2ecc)",
        },

        dim_inactive   = false,
        dim_strength   = 0.1,

        active_opacity   = 1.0,
        inactive_opacity = 0.92,
    },

    -- Layout
    dwindle = {
        preserve_split = true,
        force_split    = 2,
    },

    -- Misc
    misc = {
        disable_hyprland_logo        = true,
        disable_splash_rendering     = true,
        animate_manual_resizes       = true,
        animate_mouse_windowdragging = true,
        key_press_enables_dpms       = true,
        mouse_move_enables_dpms      = true,
    },
})

---------------------------
---- ANIMATIONS ----
---------------------------

hl.curve("overshot", { type = "bezier", points = { { 0.05, 0.9 }, { 0.1, 1.1 } } })
hl.curve("smoothOut", { type = "bezier", points = { { 0.36, 0 },   { 0.66, -0.56 } } })
hl.curve("smoothIn",  { type = "bezier", points = { { 0.25, 1 },   { 0.5, 1 } } })
hl.curve("snappy",    { type = "bezier", points = { { 0.4, 0 },    { 0.2, 1 } } })

hl.config({ animations = { enabled = true } })

-- smooth and snappy
hl.animation({ leaf = "windows",     enabled = true, speed = 1, bezier = "overshot", style = "slide" })
hl.animation({ leaf = "windowsOut",  enabled = true, speed = 1, bezier = "smoothOut", style = "slide" })
hl.animation({ leaf = "windowsMove", enabled = true, speed = 1, bezier = "snappy" })
hl.animation({ leaf = "border",      enabled = true, speed = 1, bezier = "default" })
hl.animation({ leaf = "borderangle", enabled = true, speed = 1, bezier = "default", style = "loop" })
hl.animation({ leaf = "fade",        enabled = true, speed = 1, bezier = "smoothIn" })
hl.animation({ leaf = "fadeDim",     enabled = true, speed = 1, bezier = "smoothIn" })
hl.animation({ leaf = "workspaces",  enabled = true, speed = 1, bezier = "overshot", style = "slidevert" })

---------------------------
---- WINDOW RULES ----
---------------------------

-- Float and center Calamares at a comfortable size
hl.window_rule({
    name    = "calamares-float-center",
    match   = { class = "^calamares$" },
    float   = true,
    size    = "980 640",
    center  = true,
    opacity = "1.0 1.0",
})

-- ArchTitan Quick Settings Window Rules
hl.window_rule({
    name    = "titan-quicksettings",
    match   = { class = "^titan-quicksettings$" },
    float   = true,
    pin     = true,
    opacity = "1.0",
})

-- ArchTitan Media Island -- SUPER+M toggle (uses LayerShellQt, no window rules needed)
hl.bind(mainMod .. " + M", hl.dsp.exec_cmd("titan-bar-media"))  -- Jump Waybar center viewport to Media page

-- Media keys (XF86)
hl.bind("XF86AudioPlay",         hl.dsp.exec_cmd("playerctl play-pause"))
hl.bind("XF86AudioNext",         hl.dsp.exec_cmd("playerctl next"))
hl.bind("XF86AudioPrev",         hl.dsp.exec_cmd("playerctl previous"))
hl.bind("XF86AudioRaiseVolume",  hl.dsp.exec_cmd("wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%+"))
hl.bind("XF86AudioLowerVolume",  hl.dsp.exec_cmd("wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%-"))
hl.bind("XF86AudioMute",         hl.dsp.exec_cmd("wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle"))
