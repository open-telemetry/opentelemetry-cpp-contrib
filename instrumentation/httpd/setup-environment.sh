#!/bin/bash

set -e

apt-get install --no-install-recommends --no-install-suggests -y \
                apache2 \
                apache2-dev
