import uuid
import threading
import ctypes
import json
import weakref
import time
import neuron
from neuron import h

_update_thread = None
_gui_widgets = []
_shape_plot_list = []
_active_container = None
_structure_change_count = neuron.nrn_dll_sym("structure_change_cnt", ctypes.c_int)
_diam_change_count = neuron.nrn_dll_sym("diam_change_cnt", ctypes.c_int)
_last_diam_change_count = _diam_change_count.value
_last_structure_change_count = _structure_change_count.value
update_interval = 0.1  # seconds
_has_setup = False


def _ensure_update_thread():
    global _update_thread
    if _update_thread is None:
        _update_thread = threading.Thread(target=_do_updates)
        _update_thread.daemon = True
        _update_thread.start()


def _do_updates():
    global _last_diam_change_count, _last_structure_change_count
    while True:
        old_diam_changed = h.diam_changed
        h.define_shape()
        if (
            old_diam_changed
            or h.diam_changed
            or _diam_change_count.value != _last_diam_change_count
            or _structure_change_count.value != _last_structure_change_count
        ):
            h.diam_changed = 0
            _last_diam_change_count = _diam_change_count.value
            _last_structure_change_count = _structure_change_count.value
            _do_reset_geometry()
        for obj in _gui_widgets:
            obj = obj()
            if obj is not None and obj._ready:
                obj._update()
        time.sleep(update_interval)


def _do_reset_geometry():
    h.define_shape()
    secs = list(h.allsec())
    geo = []
    for sec in secs:
        geo += _segment_3d_pts(sec)
    javascript_embedder(("set_neuron_section_data(%s);" % json.dumps(geo)))
    for sp in _shape_plot_list:
        sp = sp()
        if sp is not None:
            sp._reload_morphology()  # could probably get rid of this, but slower response
            sp._force_redraw = True
    del secs


def _ensure_setup():
    """deferred import (and webgl) loader"""
    global _has_setup, widgets
    global display, HTML, Javascript
    global setup_threejs, javascript_embedder
    global _segment_3d_pts
    global rangevars_present, _get_ptrs

    if not _has_setup:
        _has_setup = True
        import ipywidgets as widgets
        from IPython.display import display, HTML, Javascript
        from .setup_threejs import setup_threejs, javascript_embedder
        from .utilities import _segment_3d_pts
        from .rangevar import rangevars_present, _get_ptrs

        setup_threejs(
            after="""
<script>
var neuron_section_data = undefined;

function set_neuron_section_data(new_data) {
    neuron_section_data = new_data;
}

function ShapePlot(container_name) {
    this.diam_scale = 1;
    this.mode = 1;
    this.tc = new ThreeContainer(container_name);
    this.container = $('#' + container_name);
    this.section_data = undefined;
    this.vmin = -100;
    this.vmax = 100;
}

ShapePlot.prototype.update = function() {
    if (this.section_data !== neuron_section_data) {
        this.section_data = neuron_section_data;
        this.tc.clearLines();
        var my_mode = this.mode;
        function const_function(val) {
            return function(p) {return val};
        }
        function sp_interpolater(arc3d, ys, diam_scale) {
            var my_arc3d = [];
            var lo = arc3d[0];
            var hi = arc3d[arc3d.length - 1];
            var delta = hi - lo;
            for (var i = 0; i < arc3d.length; i++) {
                my_arc3d.push((arc3d[i] - lo) / delta);
            }
            return function(p) {
                for(var i = 1; i < my_arc3d.length; i++) {
                    var me = my_arc3d[i];
                    if (p < me) {
                        var last = my_arc3d[i - 1];
                        var x = (p - last) / (me - last);
                        return diam_scale * (x * ys[i] + (1 - x) * ys[i - 1]);
                    }
                }
                return ys[my_arc3d.length - 1] * diam_scale;
            }
        }
        var const_diam_f = const_function(4 * this.diam_scale);
        var my_width_rule;
        for(var i = 0; i < this.section_data.length; i++) {
            var my_segment = this.section_data[i];
            var xs = my_segment[0];
            var ys = my_segment[1];
            var zs = my_segment[2];
            var ds = my_segment[3];
            var arcs = my_segment[4];
            var geo = new THREE.Geometry();
            for(var j = 0 ; j < xs.length; j++) {
                geo.vertices.push(new THREE.Vector3(xs[j], ys[j], zs[j]));
            }
            if (this.mode == 0) {
                my_width_rule = sp_interpolater(arcs, ds, 4 * this.diam_scale);
            } else {
                my_width_rule = const_diam_f;
            }
            this.tc.makeLine(geo, my_width_rule);
        }
    }
}

ShapePlot.prototype.force_update = function() {
    this.section_data = undefined;
    this.update();
}


ShapePlot.prototype.set_diam_scale = function(diam) {
    this.diam_scale = diam;
    this.force_update();
}

ShapePlot.prototype.update_colors = function(data) {
    var vmin = this.vmin;
    var vmax = this.vmax;
    var vdelta = vmax - vmin;
    for (var i = 0; i < this.section_data.length; i++) {
        var v = data[i];
        var r, g, b;
        if (v == null) {
            r = 0;
            g = 0;
            b = 0;
        } else {
            v = (data[i] - vmin) / vdelta;
            if (v < 0) {v = 0;}
            if (v > 1) {v = 1;}
            r = v;
            b = 1 - v;
            g = 0;
        }
        var cv = this.tc.lines[i].material.uniforms.color.value;
        cv.r = r;
        cv.g = g;
        cv.b = b;
    }
}
</script>
"""
        )


class GUIWidget:
    def __init__(self, parent=None):
        _ensure_setup()
        self._ready = False
        self._index = None
        if parent is None:
            parent = _active_container
        if parent is None:
            # parent = widgets.VBox([])
            # display(parent)
            self._index = 0
        self._parent = parent
        _ensure_update_thread()
        _gui_widgets.append(weakref.ref(self))


class PlotShape(GUIWidget):
    def __init__(self):
        from .config import options

        if options["backend"] != "jupyter":
            raise Exception(
                'gui2.PlotShape currently only works with "jupyter" backend'
            )
        GUIWidget.__init__(self)
        self._uuid = str(uuid.uuid4()).replace("-", "")
        self._graph_div = widgets.Output()
        with self._graph_div:
            display(
                HTML(
                    '<div id="%s" style="width:400px; height:400px; border: 1px solid black"></div>'
                    % self._uuid
                )
            )

        self._rangevars_control = widgets.Select(options=["No coloration"], index=0)
        parent = widgets.HBox([self._graph_div, self._rangevars_control])
        display(parent)

        self._rangevars = []
        self._val = []
        self._ptrs = []
        self._old_selection = None

        display(
            HTML(
                '<script>sp%s = new ShapePlot("%s");</script>'
                % (self._uuid, self._uuid)
            )
        )
        _shape_plot_list.append(weakref.ref(self))
        self._min = -100
        self._max = 100
        self._force_redraw = False
        self._update_colors = 5
        self._ready = True

    def camera_position(self, x, y, z):
        """set the camera position"""
        javascript_embedder(
            "sp%s.tc.camera.position.set(%g, %g, %g)" % (self._uuid, x, y, z)
        )

    def _reload_morphology(self):
        javascript_embedder(("sp%s.update()" % self._uuid))

    def scale(self, low, high):
        """Sets blue (low) and red (high) values for the color scale."""
        javascript_embedder(
            "sp%s.vmin = %g; sp%s.vmax = %g;" % (self._uuid, low, self._uuid, high)
        )
        javascript_embedder(
            "sp%s.update_colors(%s)" % (self._uuid, json.dumps(self._val))
        )

    def _do_rangevar_update(self):
        rangevars = rangevars_present(list(h.allsec()))
        if len(rangevars) != len(self._rangevars) or any(
            r1 != r2 for r1, r2 in zip(rangevars, self._rangevars)
        ):
            self._rangevars = rangevars
            self._rangevars_control.options = [r["name"] for r in rangevars]
            for i, rv in enumerate(rangevars):
                if rv["name"] == self._old_selection:
                    self._rangevars_control.index = i
                    break
            self._force_redraw = True

    def _update(self):
        if not self._ready:
            return
        self._do_rangevar_update()
        if (
            self._rangevars_control.options[self._rangevars_control.index]
            != self._old_selection
            or self._force_redraw
        ):
            self._old_selection = self._rangevars_control.options[
                self._rangevars_control.index
            ]
            self._ptrs = _get_ptrs(self._rangevars[self._rangevars_control.index])
        data = [ptr[0] if ptr is not None else None for ptr in self._ptrs]
        if (
            self._val is None
            or len(self._val) != len(data)
            or any(myval != oldval for myval, oldval in zip(data, self._val))
        ):
            if self._val is not None or any(data) is not None:
                self._update_colors += 1
            self._val = data
        self._old_selection = self._rangevars_control.options[
            self._rangevars_control.index
        ]
        if self._force_redraw:
            self._reload_morphology()
        if self._update_colors or self._force_redraw:
            javascript_embedder(
                ("sp%s.update_colors(%s)" % (self._uuid, json.dumps(self._val)))
            )
            self._update_colors = min(20, max(0, self._update_colors - 1))
        self._force_redraw = False

    def variable(self, what):
        """Sets the range variable (v, ca[cyt], hh.m, etc...) to be used for the shape plot."""
        self._do_rangevar_update()
        for i, rv in enumerate(self._rangevars):
            if rv["name"] == what:
                self._rangevars_control.index = i
                self._force_redraw = True
                self._update_colors += 5
                break
        else:
            raise Exception("No such variable " + str(what))

    def show(self, m):
        """Sets the view mode m.

        If m == 0, then displays diameter.
        If m == 1, then displays centroid.
        """
        if m not in (0, 1):
            raise Exception("view mode must be 0 or 1")
        javascript_embedder(
            ("sp%s.mode=%g; sp%s.update()" % (self._uuid, m, self._uuid))
        )
        h.diam_changed = 1

    def set_constant_diameter(self, diam):
        """sets the diameter used for mode == 1 (i.e. constant diameter/centroid view)"""
        javascript_embedder(("sp%s.set_default_diameter(%g)" % (self._uuid, diam)))
