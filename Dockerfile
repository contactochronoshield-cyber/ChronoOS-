FROM alpine:latest
LABEL maintainer="ChronoShield Networks <contactochronoshield@gmail.com>"
LABEL description="ChronoOS Modular POSIX Environment & Security Suite"

RUN apk update && apk add --no-cache \
    bash \
    git \
    openssl \
    make \
    gcc \
    musl-dev \
    coreutils \
    curl \
    ncurses

WORKDIR /app
COPY . /app
RUN chmod +x *.sh security/*.sh .chrono-ui/*.sh 2>/dev/null || true

CMD ["/bin/bash", "./install.sh"]
