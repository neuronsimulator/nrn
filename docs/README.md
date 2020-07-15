## NEURON Development

### Versioning and Release
Please refer to [NEURON Versioning and Release Guidelines](./scm/guidelines/SCMGuidelines.md).

### Source and Release Management Guide
Please refer to [NEURON Source and Release Management Guide](./scm/guide/SCMGuide.md).

### Developer documentation 

#### Online
NEURON consolidated documentation is available at [https://neuronsimulator.github.io/nrn/](https://neuronsimulator.github.io/nrn/):
* user documentation (HOC, Python, tutorials, rxd)
* developer documentation (SCM, technical topics, Doxygen)

#### Local build

In order to build documentation locally, you need to pip install the [docs_requirements](docs_requirements.txt) :
```
pip3 install --user -r docs/docs_requirements.txt --upgrade
```

Then in your CMake build folder:
```
make doxygen
make sphinx
```  
That will build everything in the `build/docs` folder and you can then open `index.html` locally.

Sidenote: these actions can be observed in [.travis.yml](../.travis.yml)
