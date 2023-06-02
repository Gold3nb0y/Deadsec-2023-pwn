FROM ubuntu:22.04

RUN apt update && apt install -y socat

COPY bin/server /
COPY bin/client /

COPY init.sh /
COPY start_client.sh /
COPY start_server.sh /
