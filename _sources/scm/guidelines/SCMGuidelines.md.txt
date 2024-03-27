# NEURON Versioning and Release Guidelines

## Release Versioning

NEURON has done releases with the version scheme “MAJOR.MINOR[.PATCH]”, where MAJOR.MINOR defines the main release with all new functionalities. It would possibly be followed by a set of bug-fixing “.PATCH” releases until a next main release becomes available. Besides, NEURON would also publish installers for alpha pre-releases. Additionally, in case the user wanted to try out the very latest dev version, he would have to checkout and compile the source. With the availability of automated testing, nightly builds may be created with automatic versioning based on the closest repository tag.

Building on these initial practices, with the future in mind, one can formulate the NEURON releases requirements as:

- Release versions defined by MAJOR.MINOR.PATCH

  - MAJOR.MINOR defines the release series, with a defined functionality set.
  - .PATCH (> 0) represents a Patch Release, having received bug-fixes only.
  - MAJOR is only incremented when significant new features are added/modified or backwards compatibility cannot be guaranteed.
- Provide Pre-Release versions
    - Version id distinct from final. Use suffixes like “a” for alpha and “b” for beta [NEW]
    - Provide binary installers (including Python wheels [NEW])
- Maintain Previous Final Releases
  - All active release series should receive non-functional bug-fixes
  - At least two release series should be active: Latest and Previous-Stable.

Having two active release series (Latest and Previous-Stable) ensures that, upon a new release, “Latest (then renamed to “Stable”) keeps receiving updates, granting the user time to migrate, while “Stable” gets deprecated by default.

### Versioning Scheme

In software releasing, it is extremely important to adhere to standard conventions for two reasons:

1.  Ensure users are able to understand the meaning of the release and compare it to another release. E.g. “This version has more/less/same functionality than X, or is more/less stable”

2.  Ensure existing deployment tools keep working with our software.

For this purpose this document focuses on two main sources: Semantic Versioning 2.0, and Python’s PEP 440.

According to NEURON requirements, and building on the aforementioned conventions, a relatively simple release versioning scheme may be used:

-   Final releases should follow the scheme MAJOR.MINOR.PATCH as specified in Semantic Versioning. It matches quite well the existing NEURON practices, and therefore it should be used as the reference, namely PATCH increments must reflect bug fixes only.

-   Pre-Releases should follow the Release Life Cycle definitions and, respecting PEP 440, these should be named:

    -   Development builds: NEURON pre-alphas should use the “.dev”N suffix as MAJOR.MINOR.”dev”N. E.g.: 8.0.dev1
    -   Alpha releases: MAJOR.MINOR”a”[N], where N is mandatory for N > 1
        E.g.: 7.8a, 8.0a or 8.0a2
    -   Beta releases: MAJOR.MINOR”b”[N]. Eg.: 8.0b or 8.0b2
    -   Release Candidates: MAJOR.MINOR”rc”[N]. E.g.: 8.0rc.Note that RCs haven’t been used so far in NEURON. They are mentioned here for future reference.


Notice the fact that development builds have a dot “.” between MINOR and dev, as per PEP 440. Also, pre-release index N, when omitted, is equivalent to 1.

Given it is desirable to track, in the SCM, which revision is the exact source of a release, we advocate the use of Git tags for every release (excluding development builds which are not true releases and create an automatic version number via git describe). Guidelines on how to use the SCM effectively to maintain several versions are discussed in the next section.


## NEURON Release Management Guidelines

Building on the introduced topics on Git technology, Versioning and Branching models, we devise guidelines for Release Management in NEURON. Among the branching models, GitLab Flow fulfills basically all NEURON’s requirements. Besides being well understood, it’s a powerful model, made for online dev platforms like GitHub/GitLab.

BBP guidelines make use of tags for releases, including pre-releases. Pre-releases should receive common lightweight tags, while Final releases and a special development-beginning tag should be annotated for reasons of automated version generation using “git describe”.

GitLab Flow branch types should be used and follow the following naming:

-   Master: master
-   Feature branches: preferably prefixed by the type of contribution. E.g.: feature/name, [bug]fix/name, hotfix/[release/]name, etc. “Hotfix” suggests one is addressing a critical bug in production and it’s therefore allowed to be merged directly into a release branch. The target release may then be provided as part of the name as well.
-   Release Branches: release/MAJOR.MINOR

### Contributing

Normal Feature / Bug-fix contributors should follow the standard GitHub Flow approach with the following details in mind:

-   Ensure an Issue is open and nobody created yet a Pull Request addressing it
-   Create a feature branch, named accordingly
-   Do (small!) commits. Create a Pull-Request early on, against master - typically not later than one day of work. Follow conventions as in [Creating Changes/Pull Requests](https://docs.google.com/document/d/15KEVxrg3dcXIRjc3Jp9sX2WOJlO_yqMCeSeg3j_lKks/edit?ts=5ebd1648#heading=h.lns0alkal5ey). In case of bug fixes, please mention which versions are affected and should receive the patch.

NOTE: In the event a release requires a hotfix which does not (easily) apply to master, the Pull Request can exceptionally be open against the release. Please mention in the PR message: “RELEASE PATCH. PLEASE PORT TO MASTER” in case the bug exists in master or “RELEASE EXCLUSIVE PATCH. DONT PORT” in case it’s a bug exclusive to this version

### Managing Releases

At the beginning of the development towards a new release, a special annotated tag should be created in master named MAJOR.MINOR.dev. Development then proceeds naturally in master. This ensures that git describe works well to obtain a valid development version id.

#### From Alpha to Final Release

![](../images/image2.png)

When a considerable set of features have been implemented, cycles of pre-release -> test -> fixes -> release happen until a final release.

-   Alpha: Alpha marks the beginning of integration tests.
    -   A lightweight tag should be created in master directly in the commit which corresponds to such release. Tag name: MAJOR.MINOR”a”. E.g.: 8.0a
    - Features targeting a future version may start to be developed. However, they can’t be merged to master until we reach beta and the stabilization branch exists

-   Beta: The software must be feature complete. At this point branching must occur so that merging features for future versions (here >=v8.1) can happen in parallel to stabilization
    -   A lightweight tag should be created, named MAJOR.MINOR”b”. E.g.:8.0b
    - A branch named release/MAJOR.MINOR should be created, E.g. release/8.0
    - Bug-fixes should directly target the release branch for faster iteration.
    - With some frequency, and on every new beta, RC or final, the release branch should be merged to master.

  -   Release Candidate [optional]: The Beta phase is over and everything looks ok. If further testing is to be conducted then an RC is released with a tag named MAJOR.MINOR”rc”. Development continues like in beta. If RC is not applicable we jump to the final release.

-   Final release. When all integration testing is finished a final release is due. An annotated tag should be created, named MAJOR.MINOR.”0”, E.g. 8.0.0


#### Patch Releases

![](../images/image1.png)

After a version is released, and until it’s declared deprecated, it shall receive fixes and improvements. As mentioned in [Contributing](https://docs.google.com/document/d/15KEVxrg3dcXIRjc3Jp9sX2WOJlO_yqMCeSeg3j_lKks/edit?ts=5ebd1648#heading=h.j25n41g028t9), at this stage, patches should target master. It is therefore required to port these changes individually to the active releases they apply (e.g. patch 11 in the figure). For that, the release maintainer must cherry-pick the patch and, if necessary, make the required adjustments. However, in the case of hotfixes - critical release fixes - the process can be reversed, i.e. developed directly from the release and merged into it, allowing the release to get the fix more rapidly (figure: patch “hot-12”).

After some fixes were applied to the release branch, a PATCH release may be made available. Even though some branching models make patch releases on every commit, NEURON has followed a more traditional release cycle. As guidelines, it seems reasonable to::
-   Space releases by around two weeks if there are hotfixes
-   Otherwise adjust the period depending on the amount of fixes, not shorter than 1 month

Release Process:
-   Ensure all fixes have been ported/backported accordingly (cherry-pick)
-   Create an annotated tag for the release: MAJOR.MINOR.PATCH (PATCH>=1), E.g. 8.0.1

#### Release Deprecation

Previous releases, which will stop receiving updates, should be deprecated. This process should be explicit and announced publicly. The deprecation process shall be accompanied by a migration guide that users can use to quickly adapt their codebase to the next supported release.

NEURON, by default, should maintain the latest two releases. However, some exceptions might arise and should be considered on a case-by-case basis. Examples would be, for instance, to keep supporting the previous major version due to a large user base. Core developers may agree to deprecate the previous minor, or maintain all three.

Suggestions for this process are:

-   Announce in the website the deprecation of the given version, with a clear statement that it won’t receive further updates. Provide migration guidelines.
-   Add a commit to the release branch which introduces such info in the banner, like “This version has been deprecated”. The commit message should also mention “DEPRECATED”
-   Create a patch release

## Reference

Guidelines extracted from [NEURON Source and Release Management Guide](../guide/SCMGuide.md)

Introduction to GitLab Flow [https://docs.gitlab.com/ee/topics/gitlab_flow.html]()
