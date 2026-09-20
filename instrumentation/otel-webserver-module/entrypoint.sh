#!/bin/bash

export PATH="/usr/local/sbin:/usr/local/bin:$PATH"

exec gosu root "$@"
