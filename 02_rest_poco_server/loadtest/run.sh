#!/bin/bash
# Load test for GET /api/v1/helloworld using wrk
# Usage: ./run.sh [URL] [threads] [connections] [duration]
# Example: ./run.sh http://localhost:8080/api/v1/helloworld 4 100 30s

set -e

URL="${1:-http://localhost:8080/api/v1/helloworld}"
THREADS="${2:-4}"
CONNECTIONS="${3:-100}"
DURATION="${4:-30s}"

echo "=== Load test: $URL ==="
echo "Threads: $THREADS, Connections: $CONNECTIONS, Duration: $DURATION"
echo ""

if command -v wrk &>/dev/null; then
    wrk -t"$THREADS" -c"$CONNECTIONS" -d"$DURATION" "$URL"
else
    echo "wrk not found, using Docker..."
    # On Mac/Windows Docker, use host.docker.internal to reach host
    if [[ "$URL" == *"localhost"* ]]; then
        DOCKER_URL="${URL/localhost/host.docker.internal}"
        echo "Using $DOCKER_URL (Docker needs host.docker.internal to reach host)"
        URL="$DOCKER_URL"
    fi
    docker run --rm satishsverma/wrk:latest \
        -t"$THREADS" -c"$CONNECTIONS" -d"$DURATION" "$URL"
fi
