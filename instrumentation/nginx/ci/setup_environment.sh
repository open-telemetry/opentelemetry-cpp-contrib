#!/bin/sh

export DEBIAN_FRONTEND=noninteractive
export TZ="Europe/London"

wget https://packages.erlang-solutions.com/erlang-solutions_2.0_all.deb && dpkg -i erlang-solutions_2.0_all.deb
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | apt-key add -
add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"

apt-get update
apt-get install --no-install-recommends --no-install-suggests -y \
  apt-transport-https ca-certificates curl gnupg-agent software-properties-common \
  python3 esl-erlang elixir docker-ce docker-ce-cli containerd.io
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | apt-key add -

curl -L "https://github.com/docker/compose/releases/download/1.28.2/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose

chmod +x /usr/local/bin/docker-compose
