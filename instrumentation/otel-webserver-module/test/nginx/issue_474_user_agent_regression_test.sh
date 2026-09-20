#!/usr/bin/env bash

set -Eeuo pipefail

if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <webserver-module-archive> <nginx-version>" >&2
    exit 2
fi

archive_path="$1"
nginx_version="$2"
nginx_binary="${NGINX_BINARY:-nginx}"
test_port="${NGINX_ISSUE_474_TEST_PORT:-18080}"

for command in curl ps tar "$nginx_binary"; do
    if ! command -v "$command" >/dev/null 2>&1; then
        echo "Required command not found: $command" >&2
        exit 1
    fi
done

if [[ ! -f "$archive_path" ]]; then
    echo "Webserver module archive not found: $archive_path" >&2
    exit 1
fi

runtime_version="$($nginx_binary -v 2>&1)"
if [[ "$runtime_version" != *"nginx/$nginx_version"* ]]; then
    echo "Expected nginx/$nginx_version, but found: $runtime_version" >&2
    exit 1
fi

test_root="$(mktemp -d /tmp/otel-nginx-issue-474.XXXXXX)"
nginx_prefix="$test_root/nginx"
nginx_config="$nginx_prefix/nginx.conf"
nginx_error_log="$nginx_prefix/error.log"
nginx_pid_file="$nginx_prefix/nginx.pid"
nginx_pid=""
current_test="startup"

cleanup() {
    local exit_code=$?
    trap - EXIT

    if [[ -n "$nginx_pid" ]] && kill -0 "$nginx_pid" >/dev/null 2>&1; then
        "$nginx_binary" -p "$nginx_prefix/" -c "$nginx_config" -s quit >/dev/null 2>&1 || true
        for _ in {1..50}; do
            if ! kill -0 "$nginx_pid" >/dev/null 2>&1; then
                break
            fi
            sleep 0.1
        done
        if kill -0 "$nginx_pid" >/dev/null 2>&1; then
            kill "$nginx_pid" >/dev/null 2>&1 || true
        fi
    fi

    if [[ $exit_code -ne 0 ]]; then
        echo "FAILED: $current_test" >&2
        if [[ -f "$nginx_error_log" ]]; then
            echo "NGINX error log:" >&2
            cat "$nginx_error_log" >&2
        fi
    fi

    case "$test_root" in
        /tmp/otel-nginx-issue-474.*)
            rm -rf -- "$test_root"
            ;;
    esac

    exit "$exit_code"
}
trap cleanup EXIT

mkdir -p "$nginx_prefix"
tar -xzf "$archive_path" -C "$test_root"

sdk_root="$test_root/opentelemetry-webserver-sdk"
module_path="$sdk_root/WebServerModule/Nginx/$nginx_version/ngx_http_opentelemetry_module.so"
sdk_library_path="$sdk_root/sdk_lib/lib"

if [[ ! -f "$module_path" ]]; then
    echo "NGINX module not found in archive: $module_path" >&2
    exit 1
fi

if [[ ! -x "$sdk_root/install.sh" ]]; then
    echo "SDK installer not found in archive: $sdk_root/install.sh" >&2
    exit 1
fi

"$sdk_root/install.sh" --ignore-permissions

export LD_LIBRARY_PATH="$sdk_library_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export OTEL_SDK_LOG_CONFIG_PATH="$sdk_root/conf/opentelemetry_sdk_log4cxx.xml"

cat >"$nginx_config" <<EOF
load_module $module_path;

user root;
worker_processes 1;
pid $nginx_pid_file;
error_log $nginx_error_log notice;

events {
    worker_connections 64;
}

http {
    access_log off;

    NginxModuleEnabled ON;
    NginxModuleOtelSpanExporter osstream;
    NginxModuleOtelSpanProcessor simple;
    NginxModuleOtelSampler always_on;
    NginxModuleServiceName issue-474;
    NginxModuleServiceNamespace regression-test;
    NginxModuleServiceInstanceId ci;
    NginxModuleResolveBackends OFF;
    NginxModuleTraceAsError OFF;

    server {
        listen 127.0.0.1:$test_port;
        server_name localhost;

        location = /issue-474 {
            return 200 "ok\n";
        }
    }
}
EOF

"$nginx_binary" -t -p "$nginx_prefix/" -c "$nginx_config"
"$nginx_binary" -p "$nginx_prefix/" -c "$nginx_config"

for _ in {1..50}; do
    if [[ -s "$nginx_pid_file" ]]; then
        nginx_pid="$(<"$nginx_pid_file")"
        if curl --silent --fail --max-time 1 --http1.1 \
            -H 'User-Agent: issue-474-readiness-check' \
            "http://127.0.0.1:$test_port/issue-474" >/dev/null; then
            break
        fi
    fi
    sleep 0.1
done

if [[ -z "$nginx_pid" ]] || ! kill -0 "$nginx_pid" >/dev/null 2>&1; then
    echo "NGINX did not start" >&2
    exit 1
fi

worker_pid() {
    ps -eo pid=,ppid= | awk -v parent="$nginx_pid" '$2 == parent { print $1; exit }'
}

assert_no_worker_crash() {
    if grep -Eiq 'worker process .*exited on signal|segmentation fault|core dumped' "$nginx_error_log"; then
        echo "NGINX worker crash detected" >&2
        return 1
    fi
}

run_test_case() {
    local test_name="$1"
    shift

    current_test="$test_name"
    echo "RUN: $test_name"

    local worker_before
    local worker_after
    local http_status

    worker_before="$(worker_pid)"
    if [[ -z "$worker_before" ]]; then
        echo "Unable to find the NGINX worker before $test_name" >&2
        return 1
    fi

    if ! http_status="$(curl --silent --show-error --output /dev/null \
        --write-out '%{http_code}' --max-time 5 --http1.1 \
        "$@" "http://127.0.0.1:$test_port/issue-474")"; then
        echo "Request failed for $test_name" >&2
        return 1
    fi

    if [[ "$http_status" != "200" ]]; then
        echo "Expected HTTP 200 for $test_name, received $http_status" >&2
        return 1
    fi

    worker_after="$(worker_pid)"
    if [[ "$worker_after" != "$worker_before" ]]; then
        echo "NGINX worker changed during $test_name: $worker_before -> ${worker_after:-missing}" >&2
        return 1
    fi

    assert_no_worker_crash
    echo "PASS: $test_name"
}

test_non_empty_user_agent() {
    run_test_case "non-empty User-Agent" -H 'User-Agent: otel-regression-test'
}

test_omitted_user_agent() {
    run_test_case "omitted User-Agent" -H 'User-Agent:'
}

test_empty_user_agent() {
    run_test_case "empty User-Agent" -H 'User-Agent;'
}

test_non_empty_user_agent
test_omitted_user_agent
test_empty_user_agent

current_test="complete"
echo "All NGINX issue #474 regression tests passed"
