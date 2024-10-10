
FROM debian:bullseye-slim

RUN apt-get update && apt-get install -y \
    build-essential \
    libgpiod-dev \
    libmicrohttpd-dev \
    libcjson-dev \
    libcurl4-openssl-dev \
    curl \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

COPY . /app
WORKDIR /app

RUN make clean && make

CMD ["./NetworkSwitchController"]
