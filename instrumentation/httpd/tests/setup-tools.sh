#!/bin/sh
set -e

export DEBIAN_FRONTEND=noninteractive

apt-get install -qq netcat-traditional httperf
