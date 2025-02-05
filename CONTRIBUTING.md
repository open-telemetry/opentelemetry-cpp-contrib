# Contributing to opentelemetry-cpp-contrib

## Introduction

Welcome to the opentelemetry-cpp-contrib repository! This project is an integral part of the broader OpenTelemetry ecosystem, providing additional instrumentation and tools to enhance C/C++ observability.

We greatly appreciate any contributions, no matter the size or scope. Please feel free to reach out to the OpenTelemetry C/C++ community on [slack](https://cloud-native.slack.com/archives/C01N3AT62SJ) with questions or for assistance.

## Prerequisites

[C++14](https://github.com/open-telemetry/opentelemetry-cpp/?tab=readme-ov-file#supported-c-versions) or higher

CMake 3.18 or higher

Git

## Workflow

-Use forked repo when contributing

-Follow the OpenTelemetry C++ SDK contributing guidelines

-Write clear, concise commit messages

## Local Run/Build

To create a new PR, fork the project in GitHub and clone the upstream repo:

```sh
git clone --recursive https://github.com/open-telemetry/opentelemetry-cpp-contrib.git
```

Add your fork as a remote:

```sh
git remote add fork https://github.com/YOUR_GITHUB_USERNAME/opentelemetry-cpp-contrib.git
```

Check out a new branch, make modifications and push the branch to your fork:

```sh
git checkout -b feature
# edit files
git commit
git push fork feature
```

Open a pull request against the main `opentelemetry-cpp-contrib` repo.

### Build and Run Code Examples

TBD

## Testing

TBD

## Contributing Rules

Follow the OpenTelemetry [C++ SDK coding standards](https://github.com/open-telemetry/opentelemetry-cpp?tab=readme-ov-file#supported-c-versions)

Include tests for new features or bug fixes


## How to Receive Comments

* If the PR is not ready for review, please put `[WIP]` in the title, tag it
  as `work-in-progress`, or mark it as [`draft`](https://github.blog/2019-02-14-introducing-draft-pull-requests/).
* Make sure [CLA](https://identity.linuxfoundation.org/projects/cncf) is
  signed and CI is clear.
  
## How to Get PRs Merged

Address any reviewer 

Ensure all tests pass

The maintainers will squash and merge your commit when approved



## Further Help

The OpenTelemetry C/C++ special interest group (SIG) meets regularly. 

See the public meeting notes via the [README](https://github.com/open-telemetry/opentelemetry-cpp?tab=readme-ov-file#contributing) by following the link to the google doc.

Link to join SIG meets also available in the README

For more info and help, join the meeting or get in touch on the [CNCF Slack channel for opentelemetry-cpp](https://cloud-native.slack.com/archives/C01N3AT62SJ).  



## Useful Resources

Please refer to main OpenTelemetry C++ SDK [contributing guidelines](https://github.com/open-telemetry/opentelemetry-cpp/blob/master/CONTRIBUTING.md) for more details.
