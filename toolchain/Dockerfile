# Miyoo Mini cross toolchain, based on shauninman/union-miyoomini-toolchain
# pinned to commit cfcae30. setup-toolchain.sh picks the host-native
# (aarch64 or x86_64) toolchain automatically.
FROM debian/eol:buster-slim
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get -y update && apt-get -y install \
	bc build-essential bzip2 ca-certificates cpio git make rsync unzip wget zip \
  && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/shauninman/union-miyoomini-toolchain /tmp/union \
  && git -C /tmp/union checkout cfcae30 \
  && mkdir -p /root/workspace \
  && cp -r /tmp/union/support/. /root/ \
  && rm -rf /tmp/union
WORKDIR /root
RUN ./setup-toolchain.sh
RUN cat setup-env.sh >> .bashrc

VOLUME /root/workspace
WORKDIR /root/workspace
CMD ["/bin/bash"]
