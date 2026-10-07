use std::env;
use std::fs;
use std::io::{self, Read};
use std::os::unix::fs::PermissionsExt;
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};

const BLOCKED_NAMES: &[&str] = &[
    "gdm",
    "gdm3",
    "lightdm",
    "lightdm-webkit2-greeter",
    "lightdm-gtk-greeter",
    "lxdm",
    "xdm",
    "ly",
    "emptty",
    "slim",
    "greetd",
    "lemurs",
    "entrance",
    "tuigreet",
    "wlgreet",
    "agreety",
    "regreet",
    "qtgreet",
    "cosmic-greeter",
    "sddm-wayland-git",
];

const DM_PROVIDES: &[&str] = &["systemd-display-manager"];

fn log(msg: &str) {
    let _ = eprintln!("[ArchTitan Guard] {}", msg);
}

fn is_blocked_name(name: &str) -> bool {
    let n = name.trim().to_ascii_lowercase();
    BLOCKED_NAMES.iter().any(|b| n == *b)
}

fn package_provides_dm(pkg: &str) -> bool {
    let mut cmd = Command::new("pacman");
    cmd.args(["-Si", pkg]);
    cmd.stdout(Stdio::null());
    cmd.stderr(Stdio::null());
    match cmd.status() {
        Ok(s) if s.success() => {
            let mut out = String::new();
            let mut c = Command::new("pacman");
            c.args(["-Si", "--print-format", "%p", pkg]);
            if let Ok(mut child) = c.stdout(Stdio::piped()).spawn() {
                if let Some(mut stdout) = child.stdout.take() {
                    let _ = stdout.read_to_string(&mut out);
                }
                let _ = child.wait();
            }
            let lower = out.to_ascii_lowercase();
            for p in DM_PROVIDES {
                if lower.contains(p) {
                    return true;
                }
            }
            false
        }
        _ => false,
    }
}

fn pretransaction() -> i32 {
    let mut args: Vec<String> = env::args().skip(2).collect();
    if args.is_empty() {
        let mut stdin_buf = String::new();
        if io::stdin().read_to_string(&mut stdin_buf).is_ok() {
            for token in stdin_buf.split_whitespace() {
                if !token.is_empty() {
                    args.push(token.to_string());
                }
            }
        }
    }
    for pkg in &args {
        let name = pkg.split('@').next().unwrap_or(pkg.as_str());
        if is_blocked_name(name) || package_provides_dm(name) {
            log(&format!(
                "Refusing to install/modify '{}' (SDDM + Hyprland is the only supported display/session stack).",
                name
            ));
            return 1;
        }
    }
    0
}

fn ensure_dir(p: &Path) {
    let _ = fs::create_dir_all(p);
}

fn set_symlink_atomic(link: &Path, target: &str) -> bool {
    let dir = link.parent().unwrap_or(Path::new("/"));
    ensure_dir(dir);
    let tmp = PathBuf::from(format!("{}.archtitan-guard-tmp", link.display()));
    let _ = fs::remove_file(&tmp);
    match std::os::unix::fs::symlink(target, &tmp) {
        Ok(_) => match fs::rename(&tmp, link) {
            Ok(_) => true,
            Err(_) => {
                let _ = fs::remove_file(&tmp);
                false
            }
        },
        Err(_) => false,
    }
}

fn verify_symlink(link: &Path, expected_target: &str) -> bool {
    match fs::read_link(link) {
        Ok(t) => t.to_string_lossy() == expected_target,
        Err(_) => false,
    }
}

fn apply_enforcement() -> i32 {
    const DM_SERVICE: &str = "/etc/systemd/system/display-manager.service";
    const EXPECTED_DM: &str = "/usr/lib/systemd/system/sddm.service";

    if !verify_symlink(Path::new(DM_SERVICE), EXPECTED_DM) {
        if set_symlink_atomic(Path::new(DM_SERVICE), EXPECTED_DM) {
            log("Restored display-manager.service symlink to sddm.service");
        } else {
            log("Failed to restore display-manager.service symlink");
            return 2;
        }
    }

    let mask_targets = [
        "gdm.service",
        "lightdm.service",
        "lxdm.service",
        "xdm.service",
        "ly.service",
        "greetd.service",
        "emptty.service",
        "slim.service",
        "plasma-sddm-helper.service",
    ];
    let mask_dir = Path::new("/etc/systemd/system");
    ensure_dir(mask_dir);
    for m in &mask_targets {
        let p = mask_dir.join(m);
        if !verify_symlink(&p, "/dev/null") {
            if set_symlink_atomic(&p, "/dev/null") {
                log(&format!("Ensured {} is masked (/dev/null)", m));
            }
        }
    }

    0
}

fn verify() -> i32 {
    apply_enforcement()
}

fn apply() -> i32 {
    apply_enforcement()
}

fn main() {
    let mut args = env::args();
    let _prog = args.next();
    let cmd = args.next().unwrap_or_default();
    let code = match cmd.as_str() {
        "pretransaction" => pretransaction(),
        "verify" => verify(),
        "apply" | "enforce" => apply(),
        "version" => {
            println!("archtitan-guard 0.1.0");
            0
        }
        _ => {
            log("Unknown command");
            2
        }
    };
    std::process::exit(code);
}
