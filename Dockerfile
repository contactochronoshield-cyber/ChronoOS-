FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y \
        bash \
        build-essential \
        clang \
        gcc \
        make \
        git \
        curl \
        openssl \
        python3 \
        python3-pip \
        shellcheck \
        qemu-system-aarch64 \
        qemu-user-static \
        file \
        ca-certificates \
        coreutils \
        ncurses-bin && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . /app

RUN find . -name "*.sh" -not -path "./.git/*" -exec chmod +x {} \; || true

CMD ["make", "check"]
