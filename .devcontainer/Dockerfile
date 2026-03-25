# Используем официальный образ Python 3.11
FROM ubuntu:24.04

# Устанавливаем рабочую директорию
WORKDIR /workspace

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    wrk \
    python3 python3-pip python3-venv  python3-dev \
    libpq-dev \
    libboost-all-dev \
    libssl-dev \ 
    librdkafka-dev \
    clangd \
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

RUN cd /build && git clone https://github.com/Tencent/rapidjson.git &&\
    cd rapidjson &&\
    mkdir build &&\
    cd build &&\
    cmake .. &&\
    make &&\
    make install &&\
    cd .. &&\
    cd ..
    
RUN cd /build && git clone --recurse-submodules https://github.com/taocpp/PEGTL.git &&\
    cd PEGTL &&\
    mkdir build &&\
    cd build &&\
    cmake .. &&\
    make &&\
    make install &&\
    cd .. &&\
    cd ..

RUN cd /build && git clone --recurse-submodules https://github.com/microsoft/cppgraphqlgen.git &&\
    cd cppgraphqlgen &&\
    mkdir build &&\
    cd build &&\
    cmake -D GRAPHQL_BUILD_TESTS=OFF .. &&\
    make &&\
    make install &&\
    cd .. &&\
    cd ..
# Копируем зависимости
COPY requirements.txt .

# Устанавливаем зависимости
RUN pip install --no-cache-dir -r requirements.txt --break-system-packages

# Копируем остальные файлы проекта
COPY . .
COPY ./.devcontainer/vimpelcom.c4-varp-1.0.15.vsix /tmp/

# Открываем порт 8000 для FastAPI
EXPOSE 8000

CMD ["sleep", "infinity"]

