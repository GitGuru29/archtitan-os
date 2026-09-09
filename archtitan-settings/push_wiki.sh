#!/usr/bin/env bash
# push_wiki.sh — Pushes the local wiki/ folder to the GitHub wiki repo.
# Run this AFTER you have initialized the wiki on GitHub (at least one page saved via the web UI).
#
# Usage:
#   GITHUB_TOKEN=<your_token> ./push_wiki.sh
# Or set GITHUB_TOKEN in your environment/shell profile before running.

set -e

WIKI_DIR="/tmp/archtitan-wiki-push"
GITHUB_USER="GitGuru29"
GITHUB_REPO="Titan-Share-settings"

if [ -z "$GITHUB_TOKEN" ]; then
    echo "❌ GITHUB_TOKEN is not set. Export it before running:"
    echo "   export GITHUB_TOKEN=<your_personal_access_token>"
    exit 1
fi

WIKI_REMOTE="https://${GITHUB_USER}:${GITHUB_TOKEN}@github.com/${GITHUB_USER}/${GITHUB_REPO}.wiki.git"
LOCAL_WIKI="$(dirname "$0")/wiki"

echo "==> Cleaning up any previous temp clone..."
rm -rf "$WIKI_DIR"

echo "==> Cloning GitHub wiki repo..."
git clone "$WIKI_REMOTE" "$WIKI_DIR"

echo "==> Copying local wiki pages..."
cp "$LOCAL_WIKI"/*.md "$WIKI_DIR"/

echo "==> Staging all changes..."
cd "$WIKI_DIR"
git config user.email "archtitan@project.local"
git config user.name "ArchTitan Wiki Bot"
git add -A

echo "==> Committing..."
git commit -m "docs: upload full wiki — Audio-System, Architecture, Backend-Modules, Developer-Guide, Build-and-Deployment, Known-Issues, Power-Management, Roadmap, UI-Design, Technical-Report, Home" || echo "Nothing to commit."

echo "==> Pushing to GitHub wiki..."
git push origin master

echo ""
echo "✅ Done! View your wiki at:"
echo "   https://github.com/GitGuru29/Titan-Share-settings/wiki"
