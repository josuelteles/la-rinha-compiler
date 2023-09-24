FROM debian:stable-slim

RUN apt-get update && apt-get -y install gcc make

WORKDIR /var/rinha

COPY ./src /var/rinha

RUN make

ENTRYPOINT ["./la-rinha", "/var/rinha/source.rinha"]
