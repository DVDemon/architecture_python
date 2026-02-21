# Builder stage
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ca-certificates \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Build POCO from source (Prometheus support requires 1.12+)
ARG POCO_VERSION=1.15.0
WORKDIR /build
RUN git clone --depth 1 --branch poco-${POCO_VERSION}-release https://github.com/pocoproject/poco.git

WORKDIR /build/poco
RUN mkdir cmake-build && cd cmake-build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local \
    && cmake --build . --target install -j$(nproc)

# Build application
WORKDIR /build/app
COPY CMakeLists.txt ./
COPY src ./src/
RUN mkdir build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/usr/local \
    && cmake --build . -j$(nproc)

# Runner stage
FROM ubuntu:24.04 AS runner

# Copy POCO shared libraries from builder
COPY --from=builder /usr/local/lib/libPoco*.so* /usr/local/lib/

# Copy application binary
COPY --from=builder /build/app/build/poco_template_server /usr/local/bin/

ENV LD_LIBRARY_PATH=/usr/local/lib

ENV PORT=8080
ENV LOG_LEVEL=information
ENV JWT_SECRET=change-me-in-production

EXPOSE 8080

CMD ["poco_template_server"]
