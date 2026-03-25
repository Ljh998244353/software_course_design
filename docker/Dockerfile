FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    gcc-11 \
    g++-11 \
    cmake \
    ninja-build \
    libssl-dev \
    libhiredis-dev \
    libmariadb-dev \
    libyaml-cpp-dev \
    libspdlog-dev \
    git \
    curl \
    && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 100

WORKDIR /app

COPY . /app

RUN mkdir -p build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j$(nproc)

RUN mkdir -p /var/log/auction

EXPOSE 8080

CMD ["./build/bin/auction_server", "--config", "../config/server.yaml"]
