#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

PORT="${PORT:-8080}"
BASE_URL="${BASE_URL:-http://localhost:${PORT}}"

PGHOST="${PGHOST:-db}"
PGPORT="${PGPORT:-5432}"
PGDATABASE="${PGDATABASE:-archdb}"
PGUSER="${PGUSER:-stud}"
PGPASSWORD="${PGPASSWORD:-stud}"
JWT_SECRET="${JWT_SECRET:-SecretPassword}"
LOG_LEVEL="${LOG_LEVEL:-information}"

BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/build}"
BINARY_PATH="${BINARY_PATH:-$BUILD_DIR/poco_template_server}"

ACTION="${1:-up}"

case "$ACTION" in
  up)
    echo "Starting Poco REST server (no docker)..."

    if [[ -z "$PGHOST" ]]; then
      echo "ERROR: PGHOST is required (DB is expected to be reachable over the network)."
      echo "Example:"
      echo "  PGHOST=10.0.0.10 PGPORT=5432 PGDATABASE=poco_template PGUSER=postgres PGPASSWORD=secret $0 up"
      exit 1
    fi

    if [[ ! -x "$BINARY_PATH" ]]; then
      echo "Building server..."
      cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
      cmake --build "$BUILD_DIR" -j"$(nproc)"
    fi

    echo "Checking PostgreSQL network reachability at ${PGHOST}:${PGPORT} ..."
    for _ in $(seq 1 60); do
      if timeout 2 bash -c "cat < /dev/null > /dev/tcp/${PGHOST}/${PGPORT}" >/dev/null 2>&1; then
        echo "PostgreSQL is reachable."
        break
      fi
      sleep 1
    done

    if ! timeout 2 bash -c "cat < /dev/null > /dev/tcp/${PGHOST}/${PGPORT}" >/dev/null 2>&1; then
      echo "ERROR: PostgreSQL is not reachable at ${PGHOST}:${PGPORT}."
      exit 1
    fi

    # Ensure POCO shared libraries are discoverable in non-container environment.
    EXTRA_LD_PATH=""
    for d in /usr/local/lib /usr/local/lib64; do
      if [[ -d "$d" ]]; then
        EXTRA_LD_PATH="${d}${EXTRA_LD_PATH:+:$EXTRA_LD_PATH}"
      fi
    done
    if [[ -n "$EXTRA_LD_PATH" ]]; then
      export LD_LIBRARY_PATH="${EXTRA_LD_PATH}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    fi

    echo "Running: $BINARY_PATH"
    export PORT LOG_LEVEL JWT_SECRET PGHOST PGPORT PGDATABASE PGUSER PGPASSWORD

    # Foreground run so logs are visible.
    exec "$BINARY_PATH"
    ;;

  *)
    echo "Usage: $0 [up]"
    exit 1
    ;;
esac
