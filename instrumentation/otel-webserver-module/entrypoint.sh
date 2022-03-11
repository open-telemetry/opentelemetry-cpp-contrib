#!/bin/bash

export PATH="/opt/rh/devtoolset-7/root/usr/bin:/usr/local/sbin:/usr/local/bin:$PATH"

exec gosu root "$@"
