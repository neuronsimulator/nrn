# NEURON Software Policies


This document describes the policies for the NEURON software project, including
the NEURON software itself, NEURON support for related dependencies and the NEURON GitHub.

## Python Support Policy

### EOL

Drop Python versions once they are only receiving security fixes, unless it’s trivial to keep them alive (note that this is sooner than EOL) or if there is a compelling reason to keep them alive (e.g. a dependency that we absolutely need to support). However continuing to provide support must outweigh the the total cost of ownership (e.g. CI, packaging, technical debt etc).

##### Note on Existing releases

This does not mean we are going to automatically drop support for said Python versions in existing release branches.  We will only do this if there is a compelling reason to do so (e.g. we cannot create deliverables such as wheels or package installers).

### Deprecation

* Require an issue in the NEURON repository, open for at least 2 weeks and advertised on the NEURON Dev Slack, consulting and gathering information on any eventual specific use cases that would be affected by the deprecation.

* Require that this is flagged in a NEURON dev meeting (so up to 1 month turnaround time). 

### New Python versions

New Python versions will be supported as soon as they are released and a stable version is identified.




