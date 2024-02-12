# OpenTelemetry C++ Contrib

This repository contains set of components extending functionality of the
OpenTelemetry SDK. Instrumentation libraries, exporters, and other components
can find their home here.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md)

We meet weekly, and the time of the meeting alternates between Monday at 13:00
PT and Wednesday at 9:00 PT. The meeting is subject to change depending on
contributors' availability. Check the [OpenTelemetry community
calendar](https://github.com/open-telemetry/community#calendar)
for specific dates and Zoom meeting links.

Meeting notes are available as a public [Google
doc](https://docs.google.com/document/d/1i1E4-_y4uJ083lCutKGDhkpi3n4_e774SBLi9hPLocw/edit?usp=sharing).
For edit access, get in touch on
[Slack](https://cloud-native.slack.com/archives/C01N3AT62SJ).

[Maintainers](https://github.com/open-telemetry/community/blob/main/community-membership.md#maintainer)
([@open-telemetry/cpp-contrib-maintainers](https://github.com/orgs/open-telemetry/teams/cpp-contrib-maintainers)):

* [Lalit Kumar Bhasin](https://github.com/lalitb), Microsoft
* [Marc Alff](https://github.com/marcalff), Oracle
* [Tom Tan](https://github.com/ThomsonTan), Microsoft

[Approvers](https://github.com/open-telemetry/community/blob/main/community-membership.md#approver)
([@open-telemetry/cpp-contrib-approvers](https://github.com/orgs/open-telemetry/teams/cpp-contrib-approvers)):

* [DEBAJIT DAS](https://github.com/DebajitDas), Cisco
* [Ehsan Saei](https://github.com/esigo)
* [Johannes Tax](https://github.com/pyohannes), Grafana Labs
* [Josh Suereth](https://github.com/jsuereth), Google
* [Kumar Pratyush](https://github.com/kpratyus), Cisco
* [Max Golovanov](https://github.com/maxgolov), Microsoft
* [Siim Kallas](https://github.com/seemk), Splunk
* [Tobias Stadler](https://github.com/tobiasstadler)
* [Tomasz Rojek](https://github.com/TomRoSystems)

[Emeritus
Maintainer/Approver/Triager](https://github.com/open-telemetry/community/blob/main/community-membership.md#emeritus-maintainerapprovertriager):

* None

### Thanks to all the people who have contributed

[![contributors](https://contributors-img.web.app/image?repo=open-telemetry/opentelemetry-cpp-contrib)](https://github.com/open-telemetry/opentelemetry-cpp-contrib/graphs/contributors)

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
