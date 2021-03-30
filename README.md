# OpenTelemetry C++ Contrib

This repository contains set of components extending functionality of the
OpenTelemetry SDK. Instrumentation libraries, exporters, and other components
can find their home here.

## Contributing

For information on how to contribute, consult [the contributing
guidelines](./CONTRIBUTING.md)

## Support

This repository accepts public contributions. Individual components are
developed by numerous contributors. Approvers and maintainers of the main
OpenTelemetry C++ SDK repository are not expected to directly contribute
to every component.

GitHub `CODEOWNER`S file is a simple way to automate away some of the pain
associated with the review system on github, by automatically assigning
reviewers to a pull request based on which files were modified. Individual
components are encouraged to propose changes to `CODEOWNER`S file following
the process [described here](https://docs.github.com/en/github/creating-cloning-and-archiving-repositories/about-code-owners).

This repository is great for community supported components. Vendor specific
code that requires a higher supportability guarantees needs to be placed in
vendor's repository. Packages in vendor repositories would be prefixed with the
vendor name to signify the difference from community-supported components.
