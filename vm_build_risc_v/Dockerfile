FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=Europe/Moscow \
    LANG=en_US.UTF-8 \
    LC_ALL=en_US.UTF-8

RUN apt update && \
    apt install -y \
    build-essential \
    chrpath \
    cpio \
    debianutils \
    diffstat \
    file \
    gawk \
    gcc \
    git \
    iputils-ping \
    libacl1 \
    liblz4-tool \
    locales \
    python3 \
    python3-git \
    python3-jinja2 \
    python3-pexpect \
    python3-pip \
    python3-subunit \
    socat \
    texinfo \
    unzip \
    wget \
    xz-utils \
    zstd \
    sudo

RUN locale-gen en_US.UTF-8

RUN useradd -m -s /bin/bash builder
RUN echo "%sudo ALL=(ALL:ALL) NOPASSWD: ALL" > /etc/sudoers
RUN usermod -aG sudo builder

WORKDIR /home/builder

COPY scripts/build/build.sh /home/builder/scripts/build.sh
COPY scripts/build/run.sh /home/builder/scripts/run.sh
COPY meta-cluster /home/builder/meta-cluster
RUN chmod +x /home/builder/scripts/build.sh
RUN chmod +x /home/builder/scripts/run.sh

USER builder

CMD [ "/bin/bash" ]
