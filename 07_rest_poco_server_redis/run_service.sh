#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

PORT="${PORT:-8080}"
BASE_URL="${BASE_URL:-http://localhost:${PORT}}"

MONGO_HOST="${MONGO_HOST:-mongodb}"
MONGO_PORT="${MONGO_PORT:-27017}"
MONGO_DATABASE="${MONGO_DATABASE:-poco_template}"
REDIS_HOST="${REDIS_HOST:-cache}"
REDIS_PORT="${REDIS_PORT:-6379}"
JWT_SECRET="${JWT_SECRET:-SecretPassword}"
LOG_LEVEL="${LOG_LEVEL:-information}"

BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/build}"
BINARY_PATH="${BINARY_PATH:-$BUILD_DIR/poco_template_server}"

ACTION="${1:-up}"
if [[ "$#" -ge 1 ]]; then
  shift
fi

case "$ACTION" in
  up)
    echo "Starting Poco REST server (no docker)..."

    if [[ -z "$MONGO_HOST" ]]; then
      echo "ERROR: MONGO_HOST is required (MongoDB is expected to be reachable over the network)."
      echo "Example:"
      echo "  MONGO_HOST=10.0.0.10 MONGO_PORT=27017 MONGO_DATABASE=poco_template $0 up"
      exit 1
    fi

    # Всегда инкрементальная сборка: иначе после изменений исходников остаётся старый
    # бинарник (например без новых опций CLI), а скрипт не пересобирал бы его сам.
    echo "Configuring build (if needed)..."
    cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    echo "Building server (incremental)..."
    cmake --build "$BUILD_DIR" -j"$(nproc)"

    echo "Checking MongoDB network reachability at ${MONGO_HOST}:${MONGO_PORT} ..."
    for _ in $(seq 1 60); do
      if timeout 2 bash -c "cat < /dev/null > /dev/tcp/${MONGO_HOST}/${MONGO_PORT}" >/dev/null 2>&1; then
        echo "MongoDB is reachable."
        break
      fi
      sleep 1
    done

    if ! timeout 2 bash -c "cat < /dev/null > /dev/tcp/${MONGO_HOST}/${MONGO_PORT}" >/dev/null 2>&1; then
      echo "ERROR: MongoDB is not reachable at ${MONGO_HOST}:${MONGO_PORT}."
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
    export PORT LOG_LEVEL JWT_SECRET MONGO_HOST MONGO_PORT MONGO_DATABASE REDIS_HOST REDIS_PORT

    # Foreground run so logs are visible. Extra args (e.g. after `up --`) go to the binary.
    exec "$BINARY_PATH" "$@"
    ;;

  *)
    echo "Usage: $0 [up]"
    exit 1
    ;;
esac
