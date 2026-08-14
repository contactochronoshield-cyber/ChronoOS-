#!/data/data/com.termux/files/usr/bin/bash
set -e

pkg update -y
pkg install -y git python openssl curl

echo "ChronoOS Termux Edge platform ready."
echo "Next: configure the Chrono Edge Agent."
