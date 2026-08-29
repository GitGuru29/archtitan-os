#!/usr/bin/env bash
# shellcheck disable=SC2034

iso_name="archtitan"
iso_label="ARCHTITAN_$(date --date="@${SOURCE_DATE_EPOCH:-$(date +%s)}" +%Y%m)"
iso_publisher="ArchTitan OS"
iso_application="Arch Titan Linux Live Environment"
iso_version="$(date --date="@${SOURCE_DATE_EPOCH:-$(date +%s)}" +%Y.%m.%d)"
install_dir="arch"
buildmodes=('iso')
bootmodes=('uefi.grub')
pacman_conf="pacman.conf"
airootfs_image_type="squashfs"
airootfs_image_tool_options=('-comp' 'zstd' '-b' '1M' '-processors' '2' '-mem' '2G')
bootstrap_tarball_compression=('zstd' '-c' '-T0' '--auto-threads=logical' '--long' '-19')
file_permissions=(
  ["/etc/shadow"]="0:0:400"
  ["/etc/gshadow"]="0:0:400"
  ["/etc/sudoers.d/00-archtitan-nopasswd"]="0:0:440"
  ["/root"]="0:0:750"
  ["/root/.automated_script.sh"]="0:0:755"
  ["/root/.gnupg"]="0:0:700"
  ["/usr/local/bin/choose-mirror"]="0:0:755"
  ["/usr/local/bin/Installation_guide"]="0:0:755"
  ["/usr/local/bin/livecd-sound"]="0:0:755"
  ["/usr/local/bin/titan_hw_manager"]="0:0:755"
  ["/usr/local/bin/titan-hwm"]="0:0:755"
  ["/usr/local/bin/titan-hwm-waybar"]="0:0:755"
  ["/usr/local/bin/archtitan-session-guard"]="0:0:755"
  ["/usr/local/bin/archtitan-apply-immutable"]="0:0:755"
  ["/etc/systemd/system/archtitan-immutable-guard.service"]="0:0:644"
  ["/usr/local/bin/archtitan-initcpio-preset"]="0:0:755"
  ["/usr/bin/titanfetch"]="0:0:755"
  ["/etc/systemd/system/titan-active.slice"]="0:0:644"
  ["/etc/systemd/system/titan-background.slice"]="0:0:644"
  ["/etc/systemd/system/titan-frozen.slice"]="0:0:644"
  # Sandbox system
  ["/usr/local/bin/titan-sandboxd"]="0:0:755"
  ["/usr/local/bin/titan-exec-hook"]="0:0:755"
  ["/etc/systemd/system/titan-sandboxd.service"]="0:0:644"
  ["/etc/titan-sandbox"]="0:0:755"
  ["/etc/titan-sandbox/policies"]="0:0:755"
  ["/var/log/titan-sandbox"]="0:0:1777"
  ["/var/titan-sandbox"]="0:0:1777"
  ["/var/titan-sandbox/apps"]="0:0:1777"
)
