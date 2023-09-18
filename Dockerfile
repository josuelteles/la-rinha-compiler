FROM debian:stable-slim

RUN apt-get update && apt-get -y install gcc make

WORKDIR /rinha

COPY ./src /rinha

RUN make

ENV PATH="/rinha:$PATH"

#CMD ["./la-rinha"]
