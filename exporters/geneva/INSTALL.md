# Building OpenTelemetry Geneva exporter for Metrics

[CMake](https://cmake.org/) is the build system supported
for the build

# Prerequisite

 - The exporter requires opentelemetry-cpp (version v1.7.0 or higher) already installed.

 - [GoogleTest](https://github.com/google/googletest) framework to build and run
  the unittests.

# Build instructions using CMake

### Building as standalone CMake Project

1. Build and Install the prerequisite opentelemetry-cpp. Refer to [INSTALL.md](https://github.com/open-telemetry/opentelemetry-cpp/blob/main/INSTALL.md#build-instructions-using-cmake)
for instructions.

2. Getting the opentelementry-cpp-contrib source:

   ```console
   # Change to the directory where you want to create the code repository
   $ cd ~
   $ mkdir source && cd source
   $ git clone --recursive https://github.com/open-telemetry/opentelemetry-cpp-contrib
   Cloning into 'opentelemetry-cpp'...
   ...
   Resolving deltas: 100% (3225/3225), done.
   $
   ```

3. Navigate to the repository cloned above, and create the `CMake` build
   configuration.

   ```console
   $ cd opentelemetry-cpp-contrib
   $ cd exporters/geneva && mkdir build && cd build
   $ cmake ..
   -- The C compiler identification is GNU 9.3.0
   -- The CXX compiler identification is GNU 9.3.0
   ...
   -- Configuring done
   -- Generating done
   -- Build files have been written to: /home/<user>/source/opentelemetry-cpp-contrib/exporters/geneva/build
   $
   ```

   Some of the available cmake build variables we can use during cmake
   configuration:

   - `-DCMAKE_POSITION_INDEPENDENT_CODE=ON` : Please note that with default
     configuration, the code is compiled without `-fpic` option, so it is not
     suitable for inclusion in shared libraries. To enable the code for
     inclusion in shared libraries, this variable is used.
   - `-DBUILD_SHARED_LIBS=ON` : To build shared libraries for the targets.
   - `-DBUILD_TESTING=ON` : Build the unit-tests
   - `-DBUILD_EXAMPLE=ON`: Build the example code which generates measurements and collects/exports metrics periodically

4. Once build configuration is created, build the exporter:

   ```console
   $ cmake --build . --target all
    Scanning dependencies of target opentelemetry_exporter_geneva_metrics
    [ 33%] Building CXX object CMakeFiles/opentelemetry_exporter_geneva_metrics.dir/src/exporter.cc.o
    [ 66%] Building CXX object CMakeFiles/opentelemetry_exporter_geneva_metrics.dir/src/unix_domain_socket_data_transport.cc.o
    [100%] Linking CXX static library libopentelemetry_exporter_geneva_metrics.a
    [100%] Built target opentelemetry_exporter_geneva_metrics
   $
   ```

5. If CMake tests are built, run them with `ctest` command

   ```console
   $ ctest
   Test project /tmp/opentelemetry-cpp-contrib/exporters/geneva/build
   ...
   100% tests passed, 0 tests failed out of 380
   $
   ```

6. Optionally install the exporter's include headers and library at custom/default install location.

   ```console
   $ cmake --install . --prefix /<install_root>/
    Install the project...
    -- Install configuration: ""
    -- Installing: /<install_root>/lib/libopentelemetry_exporter_geneva_metrics.a
    -- Installing: /<install_root>/include/opentelemetry/exporters/geneva
    -- Installing: /<install_root>/include/opentelemetry/exporters/geneva/metrics
    -- Installing: /<install_root>/include/opentelemetry/exporters/geneva/metrics/connection_string_parser.h
    -- Installing: /<install_root>/include/opentelemetry/exporters/geneva/metrics/exporter.h
    -- Installing: /<install_root>/include/opentelemetry/exporters/geneva/metrics/unix_domain_socket_data_transport.h
    -- Installing: /<install_root>/include/opentelemetry/exporters/geneva/metrics/data_transport.h
    -- Installing: /<install_root>/include/opentelemetry/exporters/geneva/metrics/macros.h
    -- Installing: /<install_root>/include/opentelemetry/exporters/geneva/metrics/socket_tools.h
    -- Installing: /<install_root>/include/opentelemetry/exporters/geneva/metrics/exporter_options.h
   $
   ```

### Using shell script to build and install the Geneva exporter:

1. Create the following directory structure in the root directory of the application. Clone the
opentelemetry-cpp and opentelemetry-cpp-contrib under the `deps` directory as given below. And
copy the script and patch file from to the `tools` directory.

    ```
    # <project-repo-root>
    #    |
    #     -  tools
    #         |
    #           - build_geneva_metrics_exporter.sh
    #           - 001-geneva-metrics-exporter.patch
    #     -  deps
    #         |
    #          - open-telemetry
    #              |
    #               -   opentelemetry-cpp
    #               -   opentelemetry-cpp-contrib/exporters/geneva
    #
    ```
2. Execute `build_geneva_metrics_exporter.sh` script from the `tools` directory

    ```console
    $ cd <project-root>/tools && ./build_geneva_metrics_exporter.sh

    ```
3. This will build the opentelemetry-cpp and Geneva exporter and install them to
`/usr`. Modify the script accordingly to change the default install location.
