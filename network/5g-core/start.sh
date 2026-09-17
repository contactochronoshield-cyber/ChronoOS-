#!/bin/sh

set -e

cd "$(dirname "$0")"

if [ ! -d ".venv" ]; then
    python -m venv .venv
fi

. .venv/bin/activate

pip install -r requirements.txt

exec uvicorn api.app:app --host 0.0.0.0 --port 8050
