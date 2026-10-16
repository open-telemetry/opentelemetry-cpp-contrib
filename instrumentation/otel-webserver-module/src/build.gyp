{
  'targets': [{
    'target_name': 'opentelemetry_webserver_sdk',
    'type': 'shared_library',

    # HAVE_ABSEIL: gRPC's public headers pull in real abseil, so opentelemetry-cpp
    # must use it too rather than its vendored absl::otel_v1 snapshot.
    'defines': ['TIMER_USE_CGT', 'HAVE_ABSEIL', 'OPENTELEMETRY_PROTO_API=__attribute__((visibility("default")))'],

    'xcode_settings': {
      'OTHER_CFLAGS': [
        '-std=c++17',
        '-g',
        '-Wno-deprecated-register',
        #'-fvisibility=hidden -fvisibility-inlines-hidden -pthread -fPIC'
        '-pthread -fPIC'
      ],
      'OTHER_LDFLAGS': ['-lpthread -ldl -lz -stdlib=libstdc++']
    },

    'sources': [
      'core/api/WSAgent.cpp',
      'core/api/RequestProcessingEngine.cpp',
      'core/api/ApiUtils.cpp',
      'core/api/SpanNamer.cpp',
      'core/api/opentelemetry_ngx_api.cpp',
      'core/AgentLogger.cpp',
      'core/AgentCore.cpp',
      'core/sdkwrapper/SdkHelperFactory.cpp',
      'core/sdkwrapper/ScopedSpan.cpp',
      'core/sdkwrapper/ServerSpan.cpp',
      'core/sdkwrapper/SdkWrapper.cpp',
      'util/SpanNamingUtils.cpp',
      'util/RegexResolver.cpp'
    ],

    'conditions': [
      ['OS=="linux"', {
        'cflags': [
          '$(COMPILER_FLAGS)',
          '-pthread -fPIC',
          '-std=c++17',
          '-g',
          '-O1 -D_FORTIFY_SOURCE=1',
        ],

        'library_dirs': [
        ],
        'libraries': [
          '$(ANSDK_DIR)/apr/1.7.6/lib/libapr-1.a',
          '$(ANSDK_DIR)/apr-util/1.6.5/lib/libaprutil-1.a',
          '$(ANSDK_DIR)/expat/2.8.5/lib/libexpat.a',
          '$(ANSDK_DIR)/apache-log4cxx/1.8.0/lib/liblog4cxx.a',
          # Ubuntu's ld defaults to --as-needed and would drop these .so files, since the
          # static OTLP archives that reference them come later in the link line.
          '-Wl,--no-as-needed',
          '$(ANSDK_DIR)/opentelemetry/$(CPP_SDK_VERSION)/lib/libopentelemetry_common.so',
          '$(ANSDK_DIR)/opentelemetry/$(CPP_SDK_VERSION)/lib/libopentelemetry_resources.so',
          '$(ANSDK_DIR)/opentelemetry/$(CPP_SDK_VERSION)/lib/libopentelemetry_trace.so',
          '$(ANSDK_DIR)/opentelemetry/$(CPP_SDK_VERSION)/lib/libopentelemetry_exporter_ostream_span.so',
          # otlp_recordable.a's logs/metrics conversion code (compiled in unconditionally
          # alongside the trace recordable code) references these even though this module
          # only exports traces.
          '$(ANSDK_DIR)/opentelemetry/$(CPP_SDK_VERSION)/lib/libopentelemetry_instrumentation_scope.so',
          '$(ANSDK_DIR)/opentelemetry/$(CPP_SDK_VERSION)/lib/libopentelemetry_logs.so',
          '$(ANSDK_DIR)/opentelemetry/$(CPP_SDK_VERSION)/lib/libopentelemetry_metrics.so',
          # gRPC's own vendored protobuf builds STATIC, which forces opentelemetry-cpp's
          # exporters/otlp/CMakeLists.txt (protobuf_lib_type == STATIC_LIBRARY check) to
          # build these OTLP targets as .a rather than .so regardless of BUILD_SHARED_LIBS.
          # --start-group/--end-group avoids having to hand-resolve their mutual ordering.
          '-Wl,--start-group',
          '$(ANSDK_DIR)/opentelemetry/$(CPP_SDK_VERSION)/lib/libopentelemetry_otlp_recordable.a',
          '$(ANSDK_DIR)/opentelemetry/$(CPP_SDK_VERSION)/lib/libopentelemetry_otlp_common.a',
          '$(ANSDK_DIR)/opentelemetry/$(CPP_SDK_VERSION)/lib/libopentelemetry_exporter_otlp_grpc.a',
          '$(ANSDK_DIR)/opentelemetry/$(CPP_SDK_VERSION)/lib/libopentelemetry_exporter_otlp_grpc_client.a',
          # generated protobuf/gRPC stub code for OTLP's own .proto definitions - needed
          # explicitly now that otlp_recordable/otlp_grpc are static rather than .so.
          '$(ANSDK_DIR)/opentelemetry/$(CPP_SDK_VERSION)/lib/libopentelemetry_proto.a',
          '$(ANSDK_DIR)/opentelemetry/$(CPP_SDK_VERSION)/lib/libopentelemetry_proto_grpc.a',
          # gRPC C++ itself, and everything it needs (abseil, protobuf, upb, c-ares, re2,
          # BoringSSL/OpenSSL, zlib) - all static, installed system-wide by the gRPC build
          # step. Kept in the same start-group since the otlp_grpc archives above pull
          # symbols from these too.
          '/usr/local/lib/libgrpc++.a',
          '/usr/local/lib/libgrpc.a',
          '/usr/local/lib/libgpr.a',
          '/usr/local/lib/libaddress_sorting.a',
          # gRPC's own modular libupb_*_lib.a CMake targets embed overlapping copies of a
          # few shared object files (e.g. descriptor.upb_minitable.c.o is in both
          # libgrpc.a and libupb_descriptor_lib.a; reader.c.o/eps_copy_input_stream.c.o
          # are in both libupb_wire_lib.a and libupb_mini_descriptor_lib.a) - all
          # byte-identical (same upstream source compiled twice by CMake's per-target
          # object aggregation), so rather than excluding whole archives and losing
          # symbols only they provide (upb_Encode/upb_Decode live in wire_lib alongside
          # the duplicated reader/eps_copy code), allow the linker to just keep the
          # first copy of any duplicate.
          '-Wl,--allow-multiple-definition',
          '/usr/local/lib/libupb_base_lib.a',
          '/usr/local/lib/libupb_mem_lib.a',
          '/usr/local/lib/libupb_message_lib.a',
          '/usr/local/lib/libupb_mini_table_lib.a',
          '/usr/local/lib/libupb_mini_descriptor_lib.a',
          '/usr/local/lib/libupb_wire_lib.a',
          '/usr/local/lib/libupb_descriptor_lib.a',
          '/usr/local/lib/libupb_lex_lib.a',
          '/usr/local/lib/libupb_hash_lib.a',
          '/usr/local/lib/libupb_json_lib.a',
          '/usr/local/lib/libupb_textformat_lib.a',
          '/usr/local/lib/libupb_reflection_lib.a',
          '/usr/local/lib/libprotobuf.a',
          '/usr/local/lib/libupb.a',
          '/usr/local/lib/libutf8_range.a',
          '/usr/local/lib/libutf8_validity.a',
          '/usr/local/lib/libcares.a',
          '/usr/local/lib/libre2.a',
          '/usr/local/lib/libssl.a',
          '/usr/local/lib/libcrypto.a',
          '/usr/local/lib/libz.a',
          '/usr/local/lib/libabsl_base.a',
          '/usr/local/lib/libabsl_city.a',
          '/usr/local/lib/libabsl_civil_time.a',
          '/usr/local/lib/libabsl_cord.a',
          '/usr/local/lib/libabsl_cord_internal.a',
          '/usr/local/lib/libabsl_cordz_functions.a',
          '/usr/local/lib/libabsl_cordz_handle.a',
          '/usr/local/lib/libabsl_cordz_info.a',
          '/usr/local/lib/libabsl_cordz_sample_token.a',
          '/usr/local/lib/libabsl_crc32c.a',
          '/usr/local/lib/libabsl_crc_cord_state.a',
          '/usr/local/lib/libabsl_crc_cpu_detect.a',
          '/usr/local/lib/libabsl_crc_internal.a',
          '/usr/local/lib/libabsl_debugging_internal.a',
          '/usr/local/lib/libabsl_decode_rust_punycode.a',
          '/usr/local/lib/libabsl_demangle_internal.a',
          '/usr/local/lib/libabsl_demangle_rust.a',
          '/usr/local/lib/libabsl_die_if_null.a',
          '/usr/local/lib/libabsl_examine_stack.a',
          '/usr/local/lib/libabsl_exponential_biased.a',
          '/usr/local/lib/libabsl_failure_signal_handler.a',
          '/usr/local/lib/libabsl_flags_commandlineflag.a',
          '/usr/local/lib/libabsl_flags_commandlineflag_internal.a',
          '/usr/local/lib/libabsl_flags_config.a',
          '/usr/local/lib/libabsl_flags_internal.a',
          '/usr/local/lib/libabsl_flags_marshalling.a',
          '/usr/local/lib/libabsl_flags_parse.a',
          '/usr/local/lib/libabsl_flags_private_handle_accessor.a',
          '/usr/local/lib/libabsl_flags_program_name.a',
          '/usr/local/lib/libabsl_flags_reflection.a',
          '/usr/local/lib/libabsl_flags_usage.a',
          '/usr/local/lib/libabsl_flags_usage_internal.a',
          '/usr/local/lib/libabsl_graphcycles_internal.a',
          '/usr/local/lib/libabsl_hash.a',
          '/usr/local/lib/libabsl_hashtablez_sampler.a',
          '/usr/local/lib/libabsl_int128.a',
          '/usr/local/lib/libabsl_kernel_timeout_internal.a',
          '/usr/local/lib/libabsl_leak_check.a',
          '/usr/local/lib/libabsl_log_flags.a',
          '/usr/local/lib/libabsl_log_globals.a',
          '/usr/local/lib/libabsl_log_initialize.a',
          '/usr/local/lib/libabsl_log_internal_check_op.a',
          '/usr/local/lib/libabsl_log_internal_conditions.a',
          '/usr/local/lib/libabsl_log_internal_fnmatch.a',
          '/usr/local/lib/libabsl_log_internal_format.a',
          '/usr/local/lib/libabsl_log_internal_globals.a',
          '/usr/local/lib/libabsl_log_internal_log_sink_set.a',
          '/usr/local/lib/libabsl_log_internal_message.a',
          '/usr/local/lib/libabsl_log_internal_nullguard.a',
          '/usr/local/lib/libabsl_log_internal_proto.a',
          '/usr/local/lib/libabsl_log_internal_structured_proto.a',
          '/usr/local/lib/libabsl_log_severity.a',
          '/usr/local/lib/libabsl_log_sink.a',
          '/usr/local/lib/libabsl_low_level_hash.a',
          '/usr/local/lib/libabsl_malloc_internal.a',
          '/usr/local/lib/libabsl_periodic_sampler.a',
          '/usr/local/lib/libabsl_poison.a',
          '/usr/local/lib/libabsl_random_distributions.a',
          '/usr/local/lib/libabsl_random_internal_distribution_test_util.a',
          '/usr/local/lib/libabsl_random_internal_entropy_pool.a',
          '/usr/local/lib/libabsl_random_internal_platform.a',
          '/usr/local/lib/libabsl_random_internal_randen.a',
          '/usr/local/lib/libabsl_random_internal_randen_hwaes.a',
          '/usr/local/lib/libabsl_random_internal_randen_hwaes_impl.a',
          '/usr/local/lib/libabsl_random_internal_randen_slow.a',
          '/usr/local/lib/libabsl_random_internal_seed_material.a',
          '/usr/local/lib/libabsl_random_seed_gen_exception.a',
          '/usr/local/lib/libabsl_random_seed_sequences.a',
          '/usr/local/lib/libabsl_raw_hash_set.a',
          '/usr/local/lib/libabsl_raw_logging_internal.a',
          '/usr/local/lib/libabsl_scoped_set_env.a',
          '/usr/local/lib/libabsl_spinlock_wait.a',
          '/usr/local/lib/libabsl_stacktrace.a',
          '/usr/local/lib/libabsl_status.a',
          '/usr/local/lib/libabsl_statusor.a',
          '/usr/local/lib/libabsl_str_format_internal.a',
          '/usr/local/lib/libabsl_strerror.a',
          '/usr/local/lib/libabsl_string_view.a',
          '/usr/local/lib/libabsl_strings.a',
          '/usr/local/lib/libabsl_strings_internal.a',
          '/usr/local/lib/libabsl_symbolize.a',
          '/usr/local/lib/libabsl_synchronization.a',
          '/usr/local/lib/libabsl_throw_delegate.a',
          '/usr/local/lib/libabsl_time.a',
          '/usr/local/lib/libabsl_time_zone.a',
          '/usr/local/lib/libabsl_tracing_internal.a',
          '/usr/local/lib/libabsl_utf8_for_code_point.a',
          '/usr/local/lib/libabsl_vlog_config_internal.a',
          '-Wl,--end-group',
          # GCC 8.5's std::filesystem lives in a separate static lib, not folded into
          # libstdc++ until GCC 9+.
          '-lstdc++fs',
          '$(BOOST_LIB)',
          '$(LINKER_FLAGS)',
          '$(LIBRARY_FLAGS)',
        ],

        'include_dirs': [
          '../linux-fixed-headers',
          '$(ANSDK_DIR)/apache-log4cxx/1.8.0/include',
          '../include/util',
          '../include/core',
          '$(ANSDK_DIR)/opentelemetry/$(CPP_SDK_VERSION)/include/',
          '$(BOOST_INCLUDE)'
        ],

        'ldflags': [
          '-Wl,--exclude-libs=ALL',
          '-Wl,--gc-sections',
          '-Wl,-z,defs',
        ]
     }],

      ['OS=="win"', {
        'default_configuration': 'Debug_x64',
        'configurations': {
          'Debug': {
            'defines': ['DEBUG', '_DEBUG'],
            'msvs_settings': {
              'VCCLCompilerTool': {
                'RuntimeLibrary': 1,
                'Optimization': 0,
              },
              'VCLinkerTool': {
                'OptimizeReferences': 2,
                'EnableCOMDATFolding': 2,
                'LinkIncremental': 1,
                'GenerateDebugInformation': 'true'
              }
            }
          },

          'Release': {
            'defines': ['NDEBUG'],
            'msvs_settings': {
              'VCCLCompilerTool': {
                'RuntimeLibrary': '0',
                'Optimization': 3,
                'FavorSizeOrSpeed': 1,
                'InlineFunctionExpansion': 2,
                'WholeProgramOptimization': 'true',
                'OmitFramePointers': 'true',
                'EnableFunctionLevelLinking': 'true',
                'EnableIntrinsicFunctions': 'true'
              },
              'VCLinkerTool': {
                'OptimizeReferences': 2,
                'EnableCOMDATFolding': 2,
                'LinkIncremental': 1
              }
            }
          },

          'Debug_x64': {
            'inherit_from': ['Debug'],
            'msvs_configuration_platform': 'x64'
          },

          'Release_x64': {
            'inherit_from': ['Release'],
            'msvs_configuration_platform': 'x64'
          },

          'Debug_x86': {
            'inherit_from': ['Debug'],
            'msvs_configuration_platform': 'Win32'
          },

          'Release_x86': {
            'inherit_from': ['Release'],
            'msvs_configuration_platform': 'Win32'
          }
        },

        'libraries': [
          '$(ANSDK_DIR)/apr/1.4.5/lib/apr-1.lib',
          '$(ANSDK_DIR)/apr-util/1.3.12/lib/aprutil-1.lib',
          '$(ANSDK_DIR)/apr-util/1.3.12/lib/xml.lib',
          '$(ANSDK_DIR)/apache-log4cxx/0.10.0/lib/log4cxx.lib',
          '$(ANSDK_DIR)/zeromq/3.2.5/lib/libzmq.lib',
          '$(ANSDK_DIR)/protobuf/2.5.0/lib/libprotoc.lib',
          '$(ANSDK_DIR)/protobuf/2.5.0/lib/libprotobuf.lib',

          '$(ANSDK_DIR)/boost/1.55.0/lib/libboost_filesystem-vc120-$(OTEL_BOOST_LIB_FLAGS)-1_55.lib',
          '$(ANSDK_DIR)/boost/1.55.0/lib/libboost_atomic-vc120-$(OTEL_BOOST_LIB_FLAGS)-1_55.lib',
          '$(ANSDK_DIR)/boost/1.55.0/lib/libboost_system-vc120-$(OTEL_BOOST_LIB_FLAGS)-1_55.lib',
          '$(ANSDK_DIR)/boost/1.55.0/lib/libboost_thread-vc120-$(OTEL_BOOST_LIB_FLAGS)-1_55.lib',
          '$(ANSDK_DIR)/boost/1.55.0/lib/libboost_chrono-vc120-$(OTEL_BOOST_LIB_FLAGS)-1_55.lib',
          '$(ANSDK_DIR)/boost/1.55.0/lib/libboost_atomic-vc120-$(OTEL_BOOST_LIB_FLAGS)-1_55.lib',
          '$(ANSDK_DIR)/boost/1.55.0/lib/libboost_date_time-vc120-$(OTEL_BOOST_LIB_FLAGS)-1_55.lib',
          '$(ANSDK_DIR)/boost/1.55.0/lib/libboost_regex-vc120-$(OTEL_BOOST_LIB_FLAGS)-1_55.lib',

          'Ws2_32.lib',
          'advapi32.lib',
          'shell32.lib',
          'mswsock.lib',
          'odbc32.lib',
          'rpcrt4.lib'
        ],

        'defines': [
          'LOG4CXX_STATIC',
          'ZMQ_STATIC',
          'BOOST_NO_CXX11_TEMPLATE_ALIASES',
          'GOOGLE_PROTOBUF_NO_RTTI'
        ],

        'include_dirs': [
          '$(ANSDK_DIR)/apache-log4cxx/0.10.0/include',
          '$(ANSDK_DIR)/apr/1.4.5/include',
          '$(ANSDK_DIR)/apr-util/1.3.12/include',
          '$(ANSDK_DIR)/zeromq/3.2.5/include',
          '$(ANSDK_DIR)/protobuf/2.5.0/include',
          '$(ANSDK_DIR)/boost/1.55.0/include',
          '../include/util',
          '../include/core',
          'protos',
        ]
      }]
    ]
  }]
}

