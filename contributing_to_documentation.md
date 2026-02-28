# How to Contribute to the NEURON Documentation

This page highlights how to document educational material related to NEURON. Contributions of any kind, like fixing a typo, expanding a tutorial, adding a Jupyter notebook, or recording a video are welcome. What follows is practical guidance on how to contribute across the three main documentation formats: written text, Python notebooks and tutorials, and video content.

**Which format should I use?** Use a `.rst` file for reference material, explanations, and guides that users will search and link to. Use a `.ipynb` notebook for anything that benefits from runnable, step-by-step code with visible outputs. Use a video for walkthroughs that are easier to follow visually, such as GUI interactions or multi-step workflows. These formats are not mutually exclusive, a good tutorial often has a notebook and a supporting video.

---

## Understanding How the Documentation is Built

Before contributing, it helps to understand the documentation pipeline. The documentation is built using a combination of Sphinx (for `.rst` and `.md` files), Doxygen (for C++ API reference), and Jupyter notebooks (`.ipynb`). When a pull request is opened on GitHub, the docs are automatically built, converting executed notebooks to HTML and combining everything into the ReadTheDocs site. When you build the documentation locally using the requisite tools, you can preview your changes by opening the resulting HTML file in your browser.

To build the documentation locally before submitting, you can use Sphinx along with all the dependencies it requires. The first compilation can be a slightly lengthy process; every subsequent one will be shorter.

---

## Contributing Text Documentation

Text-based pages are written in reStructuredText (`.rst`) and live under the `docs/` directory. The structure of the documentation follows a clear hierarchy: installation, user guides, scripting references (Python and HOC), tutorials, RxD tutorials, and course materials. There is a long list of already existing material, thus ensure that when you are submitting a PR for a new file it does not already exist. When contributing a new page or editing an existing one, place the file in the appropriate subdirectory and add it to the relevant toctree in the nearest `index.rst` file so that it appears in the navigation. When submitting a PR, ensure that only the relevant files are changed and that the content of adjacent documents is not inadvertently altered.

### Best practices for text documentation

Write for a specific audience and be explicit about it early in the document. The NEURON community ranges from computational neuroscience beginners to C++ developers. A page about a Python API function should assume Python familiarity; a page aimed at new users should assume nothing beyond basic biology and a desire to simulate and emulate neuronal systems.

The existing guide pages work well precisely because they show actual commands, actual code, and actual expected output. If you are documenting a function or a class, always include at least one runnable minimal example. These examples help people across the learning spectrum understand how the code operates.

Use NEURON's own RST conventions. The existing documentation makes heavy use of `:class:`, `:func:`, `:ref:`, `:menuselection:`, `:file:`, and `:download:` roles. Using these consistently means that cross-references and the API docs stay linked and searchable. Look at existing `.rst` files for patterns before writing new ones.

Keep language precise but not overly technical. A sentence like "The Vector class stores a sequence of doubles" is more useful than "vectors store data." At the same time, do not fill documentation with filler. State what a thing is, what it does, what its parameters mean, and what caveats exist.

Screenshots are welcome but should be kept small, clearly cropped, and saved as PNG. Only use a screenshot when the visual genuinely adds information that a code block cannot convey — for example, a screenshot of the NEURON GUI's CellBuilder showing the layout of its panels is worth including because the visual structure is the point. By contrast, do not use a screenshot to show plain text output such as the result of `n.topology()` or a printed section list — a code block is preferable there because it is searchable, copyable, and takes up less space. If you want the reader to run code interactively, the content belongs in a `.ipynb` notebook rather than an `.rst` page; the `.rst` page can then link to the notebook.

---

## Contributing Jupyter Notebooks and Tutorials

The tutorials and interactive course materials live as Jupyter notebooks inside `docs/tutorials/` and `docs/rxd-tutorials/` (among other locations). These are valuable contributions for new users because they show working code alongside explanation, allowing people to understand a concept and execute it simultaneously.

Before opening a PR with a new notebook, understand what already exists. There is a comprehensive section with extensive tutorials already in place. It is important to understand how the existing structure operates before submitting: notebooks are committed to the repository without executed outputs. Every code cell in your notebook must run cleanly from top to bottom on a fresh kernel with only NEURON and standard scientific Python dependencies available. If your notebook requires additional packages, declare them at the beginning of the notebook.

### Best practices for notebooks

Start with a clear title cell and a prose introduction that states what the reader will learn, what biological or computational problem is being illustrated, and approximately how long the notebook takes to work through. This context is what distinguishes a good tutorial notebook from a code dump. The main purpose of the notebook is to educate and guide users through a particular use case of NEURON. Alternate narrative text with code cells throughout — a notebook where every cell is code and every Markdown cell is a one-liner is hard to learn from. Explain what you are about to do, do it in code, then explain what you observed in the output.

Keep cell outputs small and meaningful. A notebook that prints large amounts of debugging output or generates unlabelled plots will be confusing on ReadTheDocs. Label axes on every plot, include units, and add a brief caption in a Markdown cell immediately below each figure.

Make the biology explicit. NEURON notebooks that are purely about syntax miss the point of the simulator. At minimum, annotate what the simulated neuron or network represents, what the parameters mean biologically, and what the output means in terms of neuronal behaviour.

Test your notebook end-to-end on a clean Python environment with `pip install neuron` before submitting. Errors will be caught during the Sphinx compilation, which will fail if any notebook cell raises an exception.

Do not commit notebooks with executed outputs. Running the `notebooks-clean` make target, or using "Kernel > Restart & Clear Output" in JupyterLab, will strip outputs. The CI is responsible for re-executing the notebooks during the build. Committing outputs causes unnecessary diff noise and risks accidentally including user-specific paths or large data blobs in the repository.

If your notebook downloads external data or depends on MOD files, include all required files in the same directory or a clearly documented subdirectory. The build runs non-interactively, so anything requiring a manual step will fail.

---

## Contributing Videos

The [videos section of the NEURON documentation](https://nrn.readthedocs.io/en/latest/videos/) is primarily a curated list of recorded courses and lectures, many hosted externally on YouTube or similar platforms. Contributing a video is therefore a two-step process: creating the video itself, and then adding a reference to it in the documentation.

For the reference page, videos are documented in `.rst` files under `docs/videos/`. Each entry typically includes the video title, a brief description of what is covered, the year or course it belongs to, and an embedded link or thumbnail. Follow the existing format in those files when adding a new entry.

As part of your PR process, please work with the maintainers to get your video uploaded to the NEURON YouTube channel. If this is inappropriate in your specific case, please explain why when opening the PR.

### Best practices for creating video content to contribute:

Plan a clear scope before recording. The most effective existing NEURON course videos cover one focused topic per session — for example, how to define cell morphology. Videos are meant to cover one learnable topic; brevity and precision are recommended. A 20-minute focused video is more likely to be watched in full than a 90-minute one.

Use a script or at minimum a structured outline. Even informal walkthrough-style videos benefit from having the key steps written out in advance. This allows for continuity, making the video structured, coherent, and easy to follow.

Show your screen at a readable resolution. If you are using a terminal or a code editor, increase your font size before recording. NEURON's GUI should be shown at a scale where menu labels and parameter fields are legible. Assume the viewer is watching on a laptop display, not a large monitor.

While recording, narrate what steps you are taking and why. If you are choosing a particular parameter value, explain the reasoning — for example, whether it is biologically motivated.

Chapters and timestamps are extremely valuable. If you are uploading to YouTube, add chapter markers to the video description so viewers can jump to the specific part they need. When writing the `.rst` entry in the NEURON docs, you can reproduce these timestamps as a brief table of contents in the text.

Keep audio quality high. A viewer will tolerate imperfect screen resolution more readily than they will tolerate echo, background noise, or muffled speech. Use a decent microphone, record in a quiet room, and do a short test recording before committing to a full session.

If you are recording a live teaching session with student interaction (as many of the existing NEURON course videos are), note this clearly in the documentation entry so viewers know to expect questions and discussion rather than a linear walkthrough.

---

## General Contribution Workflow

Regardless of which type of documentation you are contributing, the process is the same. Fork the `neuronsimulator/nrn` repository on GitHub, create a branch from master, make your changes, and open a pull request. The CI will build the documentation automatically and provide a preview link. Address any reviewer comments and ensure the docs build passes before your contribution can be merged.

If you are unsure whether a contribution is suitable or how to approach a particular topic, the [NEURON Forum](https://neuron.yale.edu/phpBB) and the [GitHub Discussions](https://github.com/neuronsimulator/nrn/discussions) tab on the repository are good places to ask. The community is active and maintainers are generally responsive. Opening an issue on GitHub before writing a large piece of documentation is a good way to get early feedback and avoid duplicating effort.

Small contributions -- correcting a factual error, improving a confusing sentence, adding a missing cross-reference -- are just as valuable as large new tutorials, and they are much easier to review. If you are new to contributing, starting with a small fix is the best way to learn the workflow before tackling something larger.