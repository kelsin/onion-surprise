# Native Linux image for the headless UI preview (SDL dummy video driver).
FROM debian:bookworm-slim
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get -y update && apt-get -y install --no-install-recommends \
	gcc libc6-dev make libsdl1.2-dev libsdl-ttf2.0-dev libsdl-image1.2-dev python3-pil \
  && rm -rf /var/lib/apt/lists/*
WORKDIR /work
