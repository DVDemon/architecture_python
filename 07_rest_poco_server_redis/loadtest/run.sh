#!/bin/bash
# Complex load test:
# 1) Prepare DB for /api/v1/users (create up to 10000 rows)
# 2) PUT random users
# 3) GET users search by substrings (query params)
#
# Usage:
#   ./run.sh [base_url] [threads] [connections] [update_duration] [search_duration] [update_count]
#
# Example:
#   ./run.sh http://localhost:8080 4 100 20s 20s 1000

set -euo pipefail

BASE_URL="${1:-http://localhost:8080}"
THREADS="${2:-4}"
CONNECTIONS="${3:-100}"
UPDATE_DURATION="${4:-20s}"
SEARCH_DURATION="${5:-20s}"
UPDATE_COUNT="${6:-1000}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="$SCRIPT_DIR/.generated"
PYTHON="${PYTHON:-python3}"

echo "=== Preparing wrk environment (users up to 10000) ==="
echo "Base URL: $BASE_URL"
echo "Threads: $THREADS, Connections: $CONNECTIONS"
echo "Update phase: $UPDATE_DURATION, update_count=$UPDATE_COUNT"
echo "Search phase: $SEARCH_DURATION"

mkdir -p "$OUT_DIR"

"$PYTHON" "$SCRIPT_DIR/prepare_wrk_env.py" \
  --base-url "$BASE_URL" \
  --update-count "$UPDATE_COUNT" \
  --id-search-count "$UPDATE_COUNT" \
  --out-dir "$OUT_DIR"

UPDATES_LUA="$OUT_DIR/updates.lua"
SEARCH_LUA="$OUT_DIR/search.lua"
ID_SEARCH_LUA="$OUT_DIR/id_search.lua"

if command -v wrk &>/dev/null; then
  echo ""
  echo "=== wrk PUT updates ==="
  wrk -t"$THREADS" -c"$CONNECTIONS" -d"$UPDATE_DURATION" -s "$UPDATES_LUA" "$BASE_URL"

  echo ""
  echo "=== wrk GET searches ==="
  wrk -t"$THREADS" -c"$CONNECTIONS" -d"$SEARCH_DURATION" -s "$SEARCH_LUA" "$BASE_URL"

  echo ""
  echo "=== wrk GET user by id searches ==="
  wrk -t"$THREADS" -c"$CONNECTIONS" -d"$SEARCH_DURATION" -s "$ID_SEARCH_LUA" "$BASE_URL"
else
  echo ""
  echo "wrk not found, using Docker..."
  # On Mac/Windows Docker, use host.docker.internal to reach host
  WRK_TARGET="$BASE_URL"
  if [[ "$WRK_TARGET" == *"localhost"* ]]; then
    WRK_TARGET="${WRK_TARGET/localhost/host.docker.internal}"
  fi

  echo "Docker wrk target: $WRK_TARGET"

  docker run --rm \
    -v "$OUT_DIR:/generated" \
    satishsverma/wrk:latest \
    -t"$THREADS" -c"$CONNECTIONS" -d"$UPDATE_DURATION" -s /generated/updates.lua "$WRK_TARGET"

  docker run --rm \
    -v "$OUT_DIR:/generated" \
    satishsverma/wrk:latest \
    -t"$THREADS" -c"$CONNECTIONS" -d"$SEARCH_DURATION" -s /generated/search.lua "$WRK_TARGET"

  docker run --rm \
    -v "$OUT_DIR:/generated" \
    satishsverma/wrk:latest \
    -t"$THREADS" -c"$CONNECTIONS" -d"$SEARCH_DURATION" -s /generated/id_search.lua "$WRK_TARGET"
fi
