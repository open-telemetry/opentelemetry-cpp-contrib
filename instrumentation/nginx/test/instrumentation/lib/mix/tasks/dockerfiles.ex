defmodule Mix.Tasks.Dockerfiles do
  use Mix.Task

  @grpc_version "v1.36.4"
  @otel_cpp_version "v0.6.0"

  def run([out_dir | combos]) do
    out_dir_abs = Path.expand(out_dir)

    Enum.map(combos, fn v -> gen_dockerfile(v) end)
    |> Enum.each(fn {job, content} ->
      out_path = Path.join(out_dir_abs, filename(job))
      File.write!(out_path, content)
    end)
  end

  defp filename(%{os: os, version: version, nginx: nginx}) do
    "Dockerfile.#{os}-#{version}.#{nginx}"
  end

  defp parse_job(identifier) do
    [distro, nginx] = String.split(identifier, ":")
    [os, version] = String.split(distro, "-")
    [ver_major, ver_minor] = String.split(version, ".") |> Enum.map(&String.to_integer/1)
    %{os: os, version: version, nginx: nginx, version_major: ver_major, version_minor: ver_minor}
  end

  defp gen_dockerfile(identifier) do
    job = parse_job(identifier)

    {job,
     Enum.join(
       [
         header(job),
         apt_install_base_pkgs(job),
         custom_cmake(job),
         custom_nginx(job),
         apt_install_custom_pkgs(job),
         build_steps(job)
       ],
       "\n"
     )}
  end

  defp header(%{os: os, version: version}) do
    """
    ARG image=#{os}:#{version}
    FROM $image AS build
    """
  end

  defp default_packages() do
    [
      "build-essential",
      "autoconf",
      "libtool",
      "pkg-config",
      "ca-certificates",
      "gcc",
      "g++",
      "git",
      "libcurl4-openssl-dev",
      "libpcre3-dev",
      "gnupg2",
      "lsb-release",
      "curl",
      "apt-transport-https",
      "software-properties-common",
      "zlib1g-dev"
    ]
  end

  defp base_packages(%{nginx: "mainline"}), do: default_packages()
  defp base_packages(%{nginx: "stable", version_major: ver_maj}) when ver_maj >= 20 do
    ["nginx" | default_packages()]
  end
  defp base_packages(_), do: default_packages()

  defp base_packages_for_version(%{version_major: major}) when major >= 20, do: ["cmake"]
  defp base_packages_for_version(_), do: []

  defp custom_cmake(%{version_major: major}) when major >= 20, do: ""

  defp custom_cmake(_) do
    """
    RUN curl -o /etc/apt/trusted.gpg.d/kitware.asc https://apt.kitware.com/keys/kitware-archive-latest.asc \\
        && apt-add-repository "deb https://apt.kitware.com/ubuntu/ `lsb_release -cs` main"
    """
  end

  defp mainline_apt(), do: "http://nginx.org/packages/mainline/ubuntu"
  defp stable_apt(), do: "http://nginx.org/packages/ubuntu"

  defp custom_nginx_step(apt_url) do
    """
    RUN curl -o /etc/apt/trusted.gpg.d/nginx_signing.asc https://nginx.org/keys/nginx_signing.key \\
        && apt-add-repository "deb #{apt_url} `lsb_release -cs` nginx" \\
        && /bin/bash -c 'echo -e "Package: *\\nPin: origin nginx.org\\nPin: release o=nginx\\nPin-Priority: 900"' | tee /etc/apt/preferences.d/99nginx
    """
  end

  defp custom_nginx(%{nginx: "mainline"}) do
    custom_nginx_step(mainline_apt())
  end

  defp custom_nginx(%{nginx: "stable", os: "ubuntu", version_major: 18}) do
    custom_nginx_step(stable_apt())
  end

  defp custom_nginx(_), do: ""

  defp custom_packages_for_version(%{os: "ubuntu", nginx: "stable", version_major: 18}) do
    ["cmake", "nginx"]
  end
  defp custom_packages_for_version(%{version_major: ver_major}) when ver_major < 20, do: ["cmake"]
  defp custom_packages_for_version(_), do: []

  defp custom_packages(%{nginx: "mainline"} = job) do
    ["nginx" | custom_packages_for_version(job)]
  end

  defp custom_packages(job), do: custom_packages_for_version(job)

  defp apt_install_base_pkgs(job) do
    packages = base_packages(job) ++ base_packages_for_version(job)
    package_install(packages)
  end

  defp apt_install_custom_pkgs(job) do
    custom_packages(job)
    |> package_install()
  end

  defp package_install([]), do: ""

  defp package_install(packages) do
    """
    RUN apt-get update \\
    && DEBIAN_FRONTEND=noninteractive TZ="Europe/London" \\
       apt-get install --no-install-recommends --no-install-suggests -y \\
       #{combine(packages, " ")}
    """
  end

  defp combine(lines, sep) do
    Enum.join(lines, sep)
  end

  defp build_steps(_) do
    """
    RUN git clone --shallow-submodules --depth 1 --recurse-submodules -b #{@grpc_version} \\
      https://github.com/grpc/grpc \\
      && cd grpc \\
      && mkdir -p cmake/build \\
      && cd cmake/build \\
      && cmake \\
        -DgRPC_INSTALL=ON \\
        -DgRPC_BUILD_TESTS=OFF \\
        -DCMAKE_INSTALL_PREFIX=/install \\
        -DCMAKE_BUILD_TYPE=Release \\
        -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF \\
        -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF \\
        -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \\
        -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \\
        -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF \\
        -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF \\
        ../.. \\
      && make -j2 \\
      && make install

    RUN git clone --shallow-submodules --depth 1 --recurse-submodules -b #{@otel_cpp_version} \\
      https://github.com/open-telemetry/opentelemetry-cpp.git \\
      && cd opentelemetry-cpp \\
      && mkdir build \\
      && cd build \\
      && cmake -DCMAKE_BUILD_TYPE=Release \\
        -DCMAKE_INSTALL_PREFIX=/install \\
        -DCMAKE_PREFIX_PATH=/install \\
        -DWITH_OTLP=ON \\
        -DBUILD_TESTING=OFF \\
        -DWITH_EXAMPLES=OFF \\
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \\
        .. \\
      && make -j2 \\
      && make install

    RUN mkdir -p otel-nginx/build && mkdir -p otel-nginx/src
    COPY src otel-nginx/src/
    COPY CMakeLists.txt nginx.cmake otel-nginx/
    RUN cd otel-nginx/build \\
      && cmake -DCMAKE_BUILD_TYPE=Release \\
        -DCMAKE_PREFIX_PATH=/install \\
        -DCMAKE_INSTALL_PREFIX=/usr/share/nginx/modules \\
        .. \\
      && make -j2 \\
      && make install

    FROM scratch AS export
    COPY --from=build /otel-nginx/build/otel_ngx_module.so .

    FROM build AS run
    CMD ["/usr/sbin/nginx", "-g", "daemon off;"]
    """
  end
end
