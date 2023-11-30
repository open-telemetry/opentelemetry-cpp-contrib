# NuGet package creation process

This directory contains NuGet specification files (.nuspec).

Creating nuget packages:

- Set `PackageVersion` environment variable that defines the package version.

```console
set PackageVersion=0.4.0
```

- Run `tools/build-nuget.cmd` script to create the package(s). Packages are copied to `.\packages`
directory.

- Push selected package to nuget feed using `NuGet.exe` tool.

Learn more about native code NuGet packages [here](https://docs.microsoft.com/en-us/nuget/guides/native-packages).

## NuGet package flavors

The following packages created :

| Package Name                 | Desription                                                  | Notes |
|------------------------------|-------------------------------------------------------------|-------|
| OpenTelemetry.Cpp.Geneva     | OpenTelemetry C++ SDK with Geneva Exporter in one package   |       |
