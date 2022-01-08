FROM ubuntu:21.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y build-essential zlib1g-dev libsdl2-dev libasound2-dev git && \
    git clone https://github.com/julius-speech/julius.git && \
    cd julius && \
    ./configure --enable-words-int && \
    make -j4 && \
    make install

FROM ubuntu:21.04 AS install

RUN apt-get update && apt-get install -y libpulse0 libasound2 libgomp1 && rm -rf /var/lib/apt/lists

COPY --from=build /usr/local /usr/local
