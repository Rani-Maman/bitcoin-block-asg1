FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    gcc \
    make \
    libssl-dev \
    libssl3 \
    wget \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY mta-utils-dev.deb /tmp/
RUN dpkg -i /tmp/mta-utils-dev.deb || (apt-get update && apt-get install -f -y)

COPY decrypter.c Makefile ./

RUN make clean && make

RUN mkdir -p /var/log && chmod 755 /var/log

CMD ["./decrypter"]
