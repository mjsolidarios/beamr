#!/usr/bin/env bash
# Makes the key that signs beamr's Android releases, and prints how to hand
# it to GitHub Actions. Run it once, on your own computer.
#
#   scripts/make-android-release-key.sh [output-folder]
#
# Every future update must be signed with this same key, or phones can't
# update without uninstalling first. Keep the keystore and its password
# somewhere safe and private (a password manager), never in the repository.
set -euo pipefail

out="${1:-$HOME/beamr-release-key}"
keystore="$out/beamr-release.jks"
alias="beamr"

if [ -e "$keystore" ]; then
    echo "$keystore already exists; not replacing a release key." >&2
    exit 1
fi
command -v keytool > /dev/null || { echo "keytool not found: install a JDK (17+)." >&2; exit 1; }

mkdir -p "$out"
chmod 700 "$out"
read -r -s -p "Password for the new key (12+ characters): " password; echo
[ "${#password}" -ge 12 ] || { echo "Too short." >&2; exit 1; }
read -r -s -p "Again: " again; echo
[ "$password" = "$again" ] || { echo "They don't match." >&2; exit 1; }

keytool -genkeypair -keystore "$keystore" -alias "$alias" -keyalg RSA -keysize 4096 -validity 10000 \
    -storepass "$password" -keypass "$password" -dname "CN=beamr, O=Mark Joseph Solidarios" -noprompt
chmod 600 "$keystore"

cat <<EOF

Made $keystore. Back it up privately now.

To let GitHub Actions sign releases, add it as repository secrets
(Settings > Secrets and variables > Actions), or with the GitHub CLI:

  base64 -w0 "$keystore" | gh secret set BEAMR_KEYSTORE_B64 -R mjsolidarios/beamr
  gh secret set BEAMR_KEYSTORE_PASSWORD -R mjsolidarios/beamr   # paste the password
  gh secret set BEAMR_KEY_PASSWORD -R mjsolidarios/beamr       # same password
  gh secret set BEAMR_KEY_ALIAS -R mjsolidarios/beamr --body "$alias"

Then tag a release:  git tag v0.2.0 && git push origin v0.2.0
EOF
