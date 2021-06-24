from .plotshape import PlotShape


def set_backend(backend):
    """sets the GUI backend.

    Currently supported options:
        jupyter -- to plot within a Jupyter notebook
    """
    if backend != "jupyter":
        raise Exception("Unsupported backend")
    else:
        from .config import options

        options["backend"] = backend
