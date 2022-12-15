#!/bin/sh
# Install script for WebServer Instrumentation

usage() {
cat << EOF
  Usage: `basename $0`
  Options:
    -h,--help                     Show Help
    --ignore-permissions          Ignores lack of write permissions on log and read permissions on the sdk folder
EOF
}

canonicalNameOfThisFile=`readlink -f "$0"`
containingDir=`dirname "$canonicalNameOfThisFile"`
agentVersionId=$(cat ${containingDir}/VERSION.txt)
ignoreFilePermissions="false"
ok=1
optspec=":h-:"

while getopts "$optspec" optchar; do
    case "${optchar}" in
        -)
            case "${OPTARG}" in
                help)
                    usage
                    exit 1
                    ;;
                ignore-permissions)
                    ignoreFilePermissions="true"
                    ;;
                *)
                    ok=0
                    echo "Invalid option: '--${OPTARG}'" >&2
                    ;;
            esac
            ;;
        h)
             usage
             exit 1
             ;;
    esac
done
if [ "$ok" != 1 ]; then
    usage
    rm -f ${install_log}
    exit 1
fi
################################################################################
# Define functions
################################################################################

log() {
    echo "$@" >> ${install_log}
    if [ -z "${OTEL_AUTO_MODE}" ]; then
        echo "$@"
    fi
}

log_error() {
    echo "[Error] $@" >> ${install_log}
    echo "[Error] $@" >&2
}

fatal_error() {
    log_error $1
    cat >&2 <<EOF

Webserver Instrumentation installation has failed. Please check $install_log for
possible causes. Please also attach it when filing a bug report.
EOF

    exit 1
}

escapeForSedReplacement() {
    local __resultVarName str __result
    __resultVarName="$1"
    str="$2"
    __result=$(echo "$str" | sed 's/[/&]/\\&/g')
    eval $__resultVarName=\'$__result\'
}

checkPathIsAccessible() {
    local __resultVarName currentPath findResult __result
    __resultVarName="$1"
    currentPath="$2"
    __result=""
    while [ "${currentPath}" != "/" ] ; do
        findResult=$(find "${currentPath}" -maxdepth 0 -perm -0005 2>/dev/null)
        if [ -z "${findResult}" ] ; then
            __result="${currentPath}"
            break
        fi
        currentPath=$(dirname "${currentPath}")
    done
    eval $__resultVarName="$__result"
}

if [ -z "${OTEL_LOCATION}" ]; then
    OTEL_LOCATION=$containingDir
    OTEL_PACKAGE=0
else
    OTEL_PACKAGE=1
fi

datestamp=`date +%Y_%m_%d_%H_%M_%S 2>/dev/null`
install_log=/tmp/otel_install_${datestamp}.log
rm -f $install_log > /dev/null 2>&1
cat > $install_log <<EOF
Webserver Instrumentation installation log
Version: ${agentVersionId}
Date: `date 2>/dev/null`

Hostname: `hostname`
Location: ${OTEL_LOCATION}
System: `uname -a 2>/dev/null`
User: `id`
Environment:
################################################################################
`env | sort`
################################################################################
EOF

echo "Install script for WebServer Instrumentation ${agentVersionId}"

# Check for SELinux
getenforce=`which getenforce 2>/dev/null`
if [ -f "${getenforce}" ]; then
    selStatus=`$getenforce`
    if [ "$selStatus" = "Enforcing" ] ; then
        log "Warning: You may encounter issues with SELinux and Opentelemetry. Please refer to https://github.com/open-telemetry/opentelemetry-cpp-contrib/tree/main/instrumentation/otel-webserver-module for more details"
    fi
fi

## Installed files locations
log4cxxFile="${containingDir}/conf/opentelemetry_sdk_log4cxx.xml"

rootDirCheck=
checkPathIsAccessible rootDirCheck "${containingDir}"
if [ -n "${rootDirCheck}" ] ; then
    if [ "${ignoreFilePermissions}" = "true" ] ; then
        log "WebServer directory '${containingDir}' is not readable by all users"
    else
        log_error "WebServer directory '${containingDir}' is not readable by all users"
        fatal_error "Change the permissions of '${rootDirCheck}' or specify --ignore-permissions"
    fi
fi

agentLogDir="${containingDir}/logs"

chmod 0777 "$agentLogDir"
#chmod 0770 "${containingDir}/proxy/conf" "${containingDir}/proxy/logs" "${containingDir}/runSDKProxy.sh" "${containingDir}/proxy/runProxy"

# write log4cxx file
escapeForSedReplacement agentLogDir "${agentLogDir}"
log4cxxTemplate="${log4cxxFile}.template"
log4cxxTemplateOwner=$(stat "--format=%u" "${log4cxxTemplate}")
log4cxxTemplateGroup=$(stat "--format=%g" "${log4cxxTemplate}")
log "Writing '${log4cxxFile}'"
cat "${log4cxxTemplate}" | \
    sed -e "s/__agent_log_dir__/${agentLogDir}/g"    \
         > "${log4cxxFile}"
if [ $? -ne 0 ] ; then
    fatal_error "Unable to write '${log4cxxFile}'"
fi

chown "${log4cxxTemplateOwner}" "${log4cxxFile}"
chgrp "${log4cxxTemplateGroup}" "${log4cxxFile}"
chmod a+r "${log4cxxFile}"
