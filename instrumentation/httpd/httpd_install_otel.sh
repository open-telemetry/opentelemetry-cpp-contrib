#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# when in docker image is FROM ubuntu:18.04
APACHE_ALL_MODULES_DIR=/etc/apache2/mods-available
# TODO check: sudo a2enmod opentel

ln -fs ${SCRIPT_DIR}/opentelemetry.load ${APACHE_ALL_MODULES_DIR}
ln -fs ${SCRIPT_DIR}/opentelemetry.conf ${APACHE_ALL_MODULES_DIR}

a2enmod opentelemetry

exit $?

# TODO: fixme when in docker image FROM httpd
# HTTPD_DIR=/usr/local/apache2/modules/
