"""RangeVarPlot.plot() and PlotShape.plot() wrappers.

Dispatched directly by Object.__getattr__; these wrappers do not replace
the legacy extension's registered plotting callbacks.
"""
from .api import (
    _nrn_get_plotshape_high,
    _nrn_get_plotshape_interface,
    _nrn_get_plotshape_low,
    _nrn_get_plotshape_section_list,
    _nrn_get_plotshape_varname,
)


class _WrapperPlot:
    """Base class returned by Object's .plot dispatch; subclasses are callable."""

    __slots__ = ("_data",)

    def __init__(self, data):
        self._data = data

    def __repr__(self):
        return f"{self._data!r}.plot()"


class _RangeVarPlot(_WrapperPlot):
    """Plot the current state of a RangeVarPlot.

    Supports matplotlib (pyplot or Axes), plotly, plotnine ggplot, and
    NEURON Graph objects. Bokeh is duck-typed via .line(). Additional
    args/kwargs are passed through. Raises TypeError for unrecognised
    graph types.

    Example::

        from matplotlib import pyplot
        rvp = n.RangeVarPlot('v', dend(0), dend(1))
        rvp.plot(pyplot.gca())
        pyplot.show()
    """

    __slots__ = ()

    def __call__(self, graph, *args, **kwargs):
        from . import NEURON

        n = NEURON()
        yvec = n.Vector()
        xvec = n.Vector()
        self._data.to_vector(yvec, xvec)

        # NEURON Graph object
        if hasattr(self._data, "hname") and hasattr(graph, "hname"):
            return yvec.line(graph, xvec, *args)

        # String-based type dispatch: avoids importing optional backends that may
        # not be installed. Mirrored from neuron/__init__.py:836.
        str_type_graph = str(type(graph))

        # plotly Figure
        if str_type_graph == "<class 'plotly.graph_objs._figure.Figure'>":
            import plotly.graph_objects as go

            kwargs.setdefault("mode", "lines")
            return graph.add_trace(
                go.Scatter(x=xvec.to_python(), y=yvec.to_python(), *args, **kwargs)
            )

        # plotnine ggplot
        if str_type_graph == "<class 'plotnine.ggplot.ggplot'>":
            import pandas as pd
            import plotnine as p9

            return graph + p9.geom_line(
                *args,
                data=pd.DataFrame({"x": xvec.to_python(), "y": yvec.to_python()}),
                mapping=p9.aes(x="x", y="y"),
                **kwargs,
            )

        str_graph = str(graph)

        # plotly module
        if str_graph.startswith("<module 'plotly' from "):
            import plotly.graph_objects as go

            fig = go.Figure()
            kwargs.setdefault("mode", "lines")
            return fig.add_trace(
                go.Scatter(x=xvec.to_python(), y=yvec.to_python(), *args, **kwargs)
            )

        # plotnine module
        if str_graph.startswith("<module 'plotnine' from "):
            import pandas as pd
            import plotnine as p9

            return p9.geom_line(
                *args,
                data=pd.DataFrame({"x": xvec.to_python(), "y": yvec.to_python()}),
                mapping=p9.aes(x="x", y="y"),
                **kwargs,
            )

        # matplotlib axes or pyplot (has .plot method)
        if hasattr(graph, "plot"):
            return graph.plot(xvec.to_python(), yvec.to_python(), *args, **kwargs)

        # bokeh (has .line method)
        if hasattr(graph, "line"):
            return graph.line(xvec.to_python(), yvec.to_python(), *args, **kwargs)

        if str_type_graph == "<class 'matplotlib.figure.Figure'>":
            raise TypeError("plot to a matplotlib axis not a matplotlib figure")

        raise TypeError(f"Unable to plot to graphs of type {type(graph)}")


def _values_between(lo, hi, data):
    """Return values from data in the closed interval [lo, hi]."""
    return [v for v in data if lo <= v <= hi]


def _values_strictly_between(lo, hi, data):
    """Return values from data in the open interval (lo, hi)."""
    temp = _values_between(lo, hi, data)
    if temp and temp[0] == lo:
        temp = temp[1:]
    if temp and temp[-1] == hi:
        temp = temp[:-1]
    return temp


def _segment_3d_pts(sec):
    """For each segment in sec, return (xs, ys, zs, diams, pts).

    Ported from neuron.gui2.utilities so myneuron's Section (which doesn't
    subclass neuron's Section) can use it via the same n3d/x3d/y3d/z3d/
    diam3d/arc3d methods myneuron provides.
    """
    import numpy

    n3d = int(sec.n3d())
    length = sec.L
    if n3d < 2:
        # No valid 3D geometry; caller should have called define_shape().
        return []
    arc3d = [sec.arc3d(i) for i in range(n3d)]
    x3d = numpy.array([sec.x3d(i) for i in range(n3d)])
    y3d = numpy.array([sec.y3d(i) for i in range(n3d)])
    z3d = numpy.array([sec.z3d(i) for i in range(n3d)])
    diam3d = numpy.array([sec.diam3d(i) for i in range(n3d)])

    dx = length / sec.nseg
    result = []
    for i in range(sec.nseg):
        x_lo = i * dx
        x_hi = (i + 1) * dx
        pts = [x_lo] + _values_strictly_between(x_lo, x_hi, arc3d) + [x_hi]
        lx = list(numpy.interp(pts, arc3d, x3d))
        ly = list(numpy.interp(pts, arc3d, y3d))
        lz = list(numpy.interp(pts, arc3d, z3d))
        ld = list(numpy.interp(pts, arc3d, diam3d))
        result.append((lx, ly, lz, ld, pts))
    return result


def _get_variable_seg(seg, variable):
    """Fetch a variable's value at a segment. Accepts 'v', 'gnabar_hh',
    or 'mech.attr' strings."""
    if not isinstance(variable, str):
        return None
    try:
        if "." in variable:
            mech, var = variable.split(".", 1)
            return float(getattr(getattr(seg, mech), var))
        return float(getattr(seg, variable))
    except (AttributeError, TypeError, ValueError):
        return None


def _color_for(val, cmap, lo, hi):
    """Map a scalar value through a matplotlib colormap (0..1).

    Returns a hex string (e.g. '#a0b0c0'). Returns 'black' when val is None.
    """
    if val is None:
        return "black"
    rng = hi - lo
    if rng == 0:
        c = cmap(0.5)
    else:
        t = min(max(val, lo), hi)
        c = cmap((t - lo) / rng)
    # Return hex for consistency with real NEURON
    r, g, b = (int(round(255 * v)) for v in c[:3])
    return f"#{r:02x}{g:02x}{b:02x}"


class _PlotShapePlot(_WrapperPlot):
    """3D shape plot with per-segment color from a range variable.

    Mirrors real NEURON's PlotShape.plot for matplotlib and plotly. Extracts the
    variable name + color range from the HOC PlotShape object, then walks
    every section in the PlotShape's section_list (or allsec if none),
    building 3D line segments coloured by the variable's current value.

    Usage::

        from matplotlib import pyplot
        ps = n.PlotShape(False)
        ps.variable('v')
        ps.scale(-80, 40)
        n.finitialize(-65)
        ps.plot(pyplot)
        pyplot.show()
    """

    __slots__ = ()

    def _get_plot_data(self):
        """Return (variable_name, lo, hi, sections) from the C API
        (nrn_get_plotshape_interface/low/high/varname + allsec).

        Real NEURON's get_plotshape_data expects a Python-wrapped HOC object
        (PyObject capsule); myneuron holds raw C pointers and would be
        misread by it.
        """
        from . import NEURON

        n = NEURON()
        spi = _nrn_get_plotshape_interface(self._data._obj)
        lo = float(_nrn_get_plotshape_low(spi))
        hi = float(_nrn_get_plotshape_high(spi))
        varname_raw = _nrn_get_plotshape_varname(spi)
        var_name = varname_raw.decode("utf-8") if varname_raw else None
        # C default when ps.variable() was never called.
        if var_name == "no variable specified":
            var_name = None
        # Honor a section list given at construction (PlotShape(SectionList));
        # only the default PlotShape() falls back to all sections.
        sl_obj = _nrn_get_plotshape_section_list(spi)
        if sl_obj:
            from .object import SectionList

            secs = list(SectionList._wrap(sl_obj))
        else:
            secs = list(n.allsec())
        return var_name, lo, hi, secs

    def __call__(self, graph, *args, **kwargs):
        from . import NEURON

        n = NEURON()
        n.define_shape()

        var_name, lo, hi, sections = self._get_plot_data()

        # Same string dispatch as _RangeVarPlot.__call__.
        is_pyplot = hasattr(graph, "__name__") and graph.__name__ == "matplotlib.pyplot"
        is_figure = str(type(graph)) == "<class 'matplotlib.figure.Figure'>"
        is_plotly_module = hasattr(graph, "__name__") and graph.__name__ == "plotly"

        if is_plotly_module:
            return self._plot_plotly(sections, var_name, lo, hi, *args, **kwargs)
        if is_pyplot or is_figure:
            return self._plot_matplotlib(
                graph, sections, var_name, lo, hi, *args, **kwargs
            )
        raise NotImplementedError(
            f"PlotShape.plot: no backend registered for {type(graph)!r}. "
            "Pass matplotlib.pyplot, a matplotlib.figure.Figure, or the "
            "plotly module."
        )

    def _plot_matplotlib(
        self, graph, sections, var_name, lo, hi, line_width=2, cmap=None, **kwargs
    ):
        from matplotlib import cm
        from mpl_toolkits.mplot3d import Axes3D  # noqa: F401 — registers 3d

        if cmap is None:
            cmap = cm.cool

        # Resolve figure/axes
        if hasattr(graph, "__name__") and graph.__name__ == "matplotlib.pyplot":
            fig = graph.figure()
        else:
            fig = graph  # already a Figure
        ax = fig.add_subplot(111, projection="3d")

        ps = self._data
        try:
            mode = int(ps.show())
        except Exception:
            mode = 0

        lines = []
        for sec in sections:
            all_seg_pts = _segment_3d_pts(sec)
            for seg, (xs, ys, zs, _, _) in zip(sec, all_seg_pts):
                # mode 0 = "Show Diameter" in NEURON GUI; use seg.diam as line width.
                lw = seg.diam if mode == 0 else line_width
                (line,) = ax.plot(xs, ys, zs, "-", linewidth=lw, **kwargs)
                val = _get_variable_seg(seg, var_name) if var_name else None
                if var_name is not None and val is not None:
                    line.set_color(_color_for(val, cmap, lo, hi))
                lines.append(line)

        # Auto-aspect so equal ranges along x/y/z
        bounds = [ax.get_xlim(), ax.get_ylim(), ax.get_zlim()]
        half = max((b[1] - b[0]) / 2 for b in bounds) or 1.0
        mids = [sum(b) / 2 for b in bounds]
        ax.set_xlim(mids[0] - half, mids[0] + half)
        ax.set_ylim(mids[1] - half, mids[1] + half)
        ax.set_zlim(mids[2] - half, mids[2] + half)
        ax.set_xlabel("x")
        ax.set_ylabel("y")
        ax.set_zlabel("z")
        if var_name:
            ax.set_title(f"PlotShape: {var_name}")
        return ax

    # TODO(gap-46): harmonise line-width kwarg name between _plot_matplotlib (line_width)
    # and _plot_plotly (width). Currently they cannot be forwarded uniformly from __call__.
    # TODO(gap-46): add integration tests for plotnine and bokeh paths in _RangeVarPlot.__call__.
    # TODO(gap-46): if fig already has a 3D axes, reuse it rather than add_subplot(111) again.
    def _plot_plotly(self, sections, var_name, lo, hi, width=2, cmap=None):
        import plotly.graph_objects as go

        if cmap is None:
            from matplotlib import cm as _cm

            cmap = _cm.cool

        ps = self._data
        try:
            mode = int(ps.show())
        except Exception:
            mode = 0

        traces = []
        for sec in sections:
            all_seg_pts = _segment_3d_pts(sec)
            for seg, (xs, ys, zs, _, _) in zip(sec, all_seg_pts):
                val = _get_variable_seg(seg, var_name) if var_name else None
                col = _color_for(val, cmap, lo, hi) if var_name else "black"
                w = seg.diam if mode == 0 else width
                hover = str(seg) + (f"<br>{val:.3f}" if val is not None else "")
                traces.append(
                    go.Scatter3d(
                        x=xs,
                        y=ys,
                        z=zs,
                        name="",
                        hovertemplate=hover,
                        mode="lines",
                        line=go.scatter3d.Line(color=col, width=w),
                    )
                )
        return go.Figure(data=traces, layout={"showlegend": False})
