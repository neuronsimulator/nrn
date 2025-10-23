from neuron import h, hoc
import numpy
import neuron
from .rxdException import RxDException

regions = {}

species = {}
all_reactions = {}

rxd_builder_tab = 1
has_instantiated = False

rxd_builder_left = 20
rxd_builder_width = 550
rxd_builder_top = 100
rxd_builder_height = 400


h.load_file("stdlib.hoc")

default_region = {"morphology": "No Sections", "geometry": "Inside"}


def _instantiate_regions(regions):
    seclist_names, seclist_map = get_sectionlists()
    for name, data in zip(list(regions.keys()), list(regions.values())):
        seclist = data.get("morphology", "No Sections")
        if seclist == "No Sections":
            seclist = "[]"
        elif seclist == "All Sections":
            seclist = "h.allsec()"
        elif seclist in seclist_map:
            seclist = f"h.SectionList[{seclist_map[seclist]}]"
        else:
            raise RxDException(f"Invalid region: {data!r}")
        geo = data.get("geometry", None)
        # TODO: do this without if statements in a way that lets geometries register themselves
        if geo is None:
            raise RxDException(f"Missing geometry for region {data!r}")
        if geo == "Inside":
            geo = "inside"
        elif geo == "Fractional Volume":
            geo = f"FractionalVolume(volume_fraction={data.get('vf', 1):g}, surface_fraction={data.get('sf', 0):g}, neighbor_areas_fraction={data.get('nf', 1):g})"
        elif geo == "Constant 2D Area/Length":
            geo = f"FixedPerimeter({data.get('perim', 1):g}, on_cell_surface={data.get('onsurf', False)!r})"
        elif geo == "Constant 3D Vol/Length":
            geo = f"FixedCrossSection({data.get('carea', 1):g}, surface_area={data.get('csurf', 0):g})"
        elif geo == "Membrane":
            geo = "membrane"
        elif geo == "Shell":
            geo = f"Shell(lo={data.get('lo', 0):g}, hi={data.get('hi', 1):g})"
        else:
            raise RxDException(f"Unrecognized geometry: {geo}")
        nrn_region = data.get("nrn_region", None)
        command = f"{name} = rxd.Region({seclist}, geometry=rxd.{geo}, nrn_region={nrn_region!r})"
        exec(command, globals())


def _instantiate_species(species):
    for name, data in zip(list(species.keys()), list(species.values())):
        regions = data.get("regions", [])
        regions = f"[{','.join(regions)}]"
        command = f'{name} = rxd.Species({regions}, d={data.get("d", 0):g}, name="{name}", charge={data.get("charge", 0):g})'
        exec(command, globals())


def _construct_side(items):
    result = []
    for term, multiplicity in items:
        if multiplicity == 1:
            result.append(term)
        elif multiplicity != 0:
            result.append(f"{multiplicity:g} * {term}")
    return " + ".join(result)


def _construct_schema(lhs, rhs):
    return f"{_construct_side(lhs)} <> {_construct_side(rhs)}"


def _instantiate_reactions(reactions):
    for name, data in zip(list(reactions.keys()), list(reactions.values())):
        reaction_spec = ""
        if "type" not in data:
            raise RxDException(f"No reaction type for: {data!r}")
        if "active" not in data or not data["active"]:
            continue
        kind = data["type"].lower()
        if kind == "rate":
            reaction_spec = f"Rate({data['species']}, {data['kf']})"
        elif kind == "multicompartmentreaction":
            reaction_spec = f"MultiCompartmentReaction({_construct_schema(data['lhs'], data['rhs'])}, {data.get('kf', 0)}, {data.get('kb', 0)}, membrane={data['membrane']}, custom_dynamics={not data.get('massaction', True)!r}, membrane_flux={data.get('membrane_current', False)!r}, scale_by_area={data.get('scale_with_area', True)!r})"
        elif kind == "reaction":
            reaction_spec = f"Reaction({_construct_schema(data['lhs'], data['rhs'])}, {data.get('kf', 0)}, {data.get('kb', 0)}, custom_dynamics={not data.get('massaction', True)!r})"
        command = f"{name} = rxd.{reaction_spec}"
        exec(command, globals())


def _instantiate():
    global has_instantiated
    if has_instantiated:
        h.continue_dialog("Cannot reinstantiate yet.")
    else:
        has_instantiated = True
        exec("from neuron import rxd", globals())
        _instantiate_regions(regions)
        _instantiate_species(species)
        _instantiate_reactions(all_reactions)


def get_sectionlists():
    found_sectionlists = {}
    results = []
    result_mapping = {}

    # start with the HOC names
    for item in dir(h):
        if item == "nseg":
            continue
        try:
            name = getattr(h, item).hname()
        except:
            continue
        if name[:12] != "SectionList[":
            continue
        found_sectionlists[name] = 0
        result_mapping[item] = int(name[12:].split("]")[0])
        results.append(item)

    # add Python names, if any
    # TODO: is there a better place to look besides globals?
    g = globals()
    for item, value in zip(list(g.keys()), list(g.values())):
        if type(value) == hoc.HocObject:
            try:
                name = value.hname()
            except:
                pass
            else:
                if name[:12] == "SectionList[":
                    found_sectionlists[name] = 0
                    result_mapping[item] = int(name[12:].split("]")[0])
                    results.append(item)

    # sort these
    results = sorted(results, key=lambda s: s.lower())

    # end with the anonymous lists (this probably includes things from cellbuilder)

    # handle any sectionlists without good hoc names
    ell = h.List("SectionList")
    for i in range(int(ell.count())):
        name = ell.object(i).hname()
        if name not in found_sectionlists:
            result_mapping[name] = int(name[12:].split("]")[0])
            results.append(name)

    return results, result_mapping


class _PartialSelector:
    def select(self, name):
        try:
            i = self.names.index(name)
            self.ell.select(i)
            self.currently_selected = name
        except:
            self.ell.select(-1)

    def selected(self):
        i = int(self.ell.selected())
        if i == -1:
            return None
        return self.names[i]

    def select_action(self, action):
        self.ell.select_action(action)


# this is a way of faking the Singleton pattern
def ReactionEditor():
    _the_reaction_editor = RxDBuilder()._the_reaction_editor
    if not _the_reaction_editor.is_mapped:
        _the_reaction_editor.map()
    return _the_reaction_editor


_the_morphology_pane = None


def MorphologyPane():
    global _the_morphology_pane
    if _the_morphology_pane is None:
        _the_morphology_pane = _MorphologyPane()
    _the_morphology_pane.map()
    return _the_morphology_pane


def RxDBuilder(left=None, top=None, width=None, height=None, visible=True):
    global _the_rxd_builder, rxd_builder_left, rxd_builder_width, rxd_builder_height, rxd_builder_top
    if left is not None:
        rxd_builder_left = left
    if top is not None:
        rxd_builder_top = top
    if width is not None:
        rxd_builder_width = width
    if height is not None:
        rxd_builder_height = height
    if _the_rxd_builder is None:
        _the_rxd_builder = _RxDBuilder()
    if not _the_rxd_builder.is_mapped and visible:
        _the_rxd_builder.map()
    return _the_rxd_builder


class FractionalVolumeOptions:
    def __init__(self):
        self.set_options({})
        h.xpanel("")
        h.xvalue("Volume Fraction", (self, "vf"))
        h.xvalue("Surface Fraction", (self, "sf"))
        h.xvalue("Neighbor Fraction", (self, "nf"))
        h.xpanel()

    def get_options(self):
        return {"vf": self.vf, "nf": self.nf, "sf": self.sf}

    def set_options(self, opt):
        self.vf = opt.get("vf", 1)
        self.nf = opt.get("nf", 1)
        self.sf = opt.get("sf", 1)

    info = (
        "Fractional Volume is used to represent regions that are intermixed\n"
        "in a nontrivial way.\n"
        "\n"
        "It is likely that in most cases the volume fraction and the\n"
        "neighbor fraction will both represent the cross-sectional area\n"
        "fraction, and so therefore should be equal. The surface fraction\n"
        "is the fraction of the surface area that belongs to this region.\n"
        "For example, if this is used to represent the ER, then the\n"
        "surface fraction should be zero."
    )


class GeoSelector:
    text = {
        1: "Inside",
        2: "Membrane",
        3: "Fractional Volume",
        4: "Shell",
        5: "Constant 2D Area/Length",
        6: "Constant 3D Vol/Length",
    }

    def select_by_name(self, name):
        click_map = {
            "Inside": self.click_inside,
            "Membrane": self.click_membrane,
            "Fractional Volume": self.click_fractional,
            "Shell": self.click_shell,
            "Constant 2D Area/Length": self.click_constarea,
            "Constant 3D Vol/Length": self.click_constvol,
        }
        click_map.get(name, self.clear_checkboxes)()

    def clear_checkboxes(self):
        self.inside_checkbox = 0
        self.membrane_checkbox = 0.0
        self.fractional_checkbox = 0.0
        self.shell_checkbox = 0.0
        self.constantarea_checkbox = 0.0
        self.constantvol_checkbox = 0.0

    def __init__(self):
        self.clear_checkboxes()
        self.callback = _do_nothing
        self.option = 1
        self.inside_checkbox = 1
        h.xpanel("")
        h.xcheckbox("Inside", (self, "inside_checkbox"), self.click_inside)
        h.xcheckbox("Membrane", (self, "membrane_checkbox"), self.click_membrane)
        h.xcheckbox(
            "Fractional Volume", (self, "fractional_checkbox"), self.click_fractional
        )
        h.xcheckbox("Shell", (self, "shell_checkbox"), self.click_shell)
        h.xcheckbox(
            "Constant 2D Area/Length",
            (self, "constantarea_checkbox"),
            self.click_constarea,
        )
        h.xcheckbox(
            "Constant 3D Vol/Length",
            (self, "constantvol_checkbox"),
            self.click_constvol,
        )
        h.xpanel()

    def click_inside(self):
        self.clear_checkboxes()
        self.inside_checkbox = 1
        self.option = 1
        self.callback()

    def click_membrane(self):
        self.clear_checkboxes()
        self.membrane_checkbox = 1
        self.option = 2
        self.callback()

    def click_fractional(self):
        self.clear_checkboxes()
        self.fractional_checkbox = 1
        self.option = 3
        self.callback()

    def click_shell(self):
        self.clear_checkboxes()
        self.shell_checkbox = 1
        self.option = 4
        self.callback()

    def click_constarea(self):
        self.clear_checkboxes()
        self.constantarea_checkbox = 1
        self.option = 5
        self.callback()

    def click_constvol(self):
        self.clear_checkboxes()
        self.constantvol_checkbox = 1
        self.option = 6
        self.callback()


class InsideOptions:
    def __init__(self):
        h.xpanel("")
        h.xlabel("No options.")
        h.xpanel()

    info = "The interior of the selected portions of the cell."

    def get_options(self):
        return {}

    def set_options(self, opt):
        pass

    is_boundary = False

    def is_valid(self):
        return True


class MembraneOptions:
    def __init__(self):
        h.xpanel("")
        h.xlabel("No options.")
        h.xpanel()

    info = "The membrane of the selected portions of the cell."

    def get_options(self):
        return {}

    def set_options(self, opt):
        pass

    is_boundary = True

    def is_valid(self):
        return True


class ShellOptions:
    def __init__(self):
        self.set_options({})
        h.xpanel("")
        h.xvalue("lo", (self, "lo"))
        h.xvalue("hi", (self, "hi"))
        h.xpanel()

    info = (
        "Shells. lo and hi denote fractional distances along the radius,\n"
        "with the center of the dendrite as 0 and the surface as 1."
    )

    def get_options(self):
        return {"lo": self.lo, "hi": self.hi}

    def set_options(self, opt):
        self.lo = opt.get("lo", 0)
        self.hi = opt.get("hi", 1)

    is_boundary = False

    def is_valid(self):
        return True


class ConstAreaOptions:
    def __init__(self):
        h.xpanel("")
        h.xlabel("Const area options.")
        h.xpanel()

    info = "A fixed membrane size."

    def __init__(self):
        self.set_options({})
        h.xpanel("")
        h.xvalue("Perimeter", (self, "perim"))
        h.xcheckbox("On surface?", (self, "onsurf"))
        h.xpanel()

    def get_options(self):
        return {"perim": self.perim, "onsurf": self.onsurf}

    def set_options(self, opt):
        self.perim = opt.get("perim", 1)
        self.onsurf = opt.get("onsurf", 0)

    is_boundary = True

    def is_valid(self):
        return True


class ConstVolOptions:
    # rxd.FixedCrossSection
    def __init__(self):
        self.set_options({})
        h.xpanel("")
        h.xvalue("Cross-Sectional Area", (self, "carea"))
        h.xvalue("Surface Area/length", (self, "csurf"))
        h.xpanel()

    def get_options(self):
        return {"csurf": self.csurf, "carea": self.carea}

    def set_options(self, opt):
        self.csurf = opt.get("csurf", 0)
        self.carea = opt.get("carea", 1)

    is_boundary = False
    info = (
        "Use constant volume only when you want to neglect diameter effects,\n"
        "such as for point models. In particular, avoid this geometry when the\n"
        "diameter varies."
    )

    def is_valid(self):
        return True


class RegionPane:
    def __init__(self):
        self.hbox1 = h.HBox(3)
        self.hbox1.intercept(1)
        self.vbox1 = h.VBox(3)
        self.vbox1.intercept(1)
        self.region_list = RegionList(None)
        h.xpanel("")
        h.xbutton("Delete", self.delete)
        h.xpanel()
        self.vbox1.intercept(0)
        self.vbox1.map()
        self.vbox2 = h.VBox(3)
        self.vbox2.intercept(1)
        h.xpanel("")
        h.xlabel("Name:")
        h.xpanel()
        self.vbox2.adjuster(15)
        self.name_editor = h.TextEditor("", 1, 30)
        self.name_editor.map()
        self.nrnregion_selector = NrnRegionSelector()
        h.xpanel("")
        h.xlabel("Select Geometry:")
        h.xpanel()
        self.hbox2 = h.HBox(3)
        self.hbox2.intercept(1)
        self.geoselector = GeoSelector()
        self.geoselector.callback = self.geo_switched
        self.deck = h.Deck(3)
        self.deck.intercept(True)
        self.option_constructors = [
            InsideOptions,
            MembraneOptions,
            FractionalVolumeOptions,
            ShellOptions,
            ConstAreaOptions,
            ConstVolOptions,
        ]
        self.option_panels = []
        for panel in self.option_constructors:
            v = h.VBox(3)
            v.intercept(1)
            self.option_panels.append(panel())
            # texteditor persists even if not saved
            spacer = h.TextEditor("", 1, 30)
            spacer.readonly(1)
            spacer.map()
            h.xpanel("")
            h.xbutton("Information", self.info)
            h.xpanel()
            v.intercept(0)
            v.map()
            # vboxes persist even when not saving a reference
        self.deck.intercept(False)
        self.deck.flip_to(0)
        self.deck.map()
        self.hbox2.intercept(0)
        self.hbox2.map()
        h.xpanel("", 1)
        h.xbutton("Revert", self.update)
        h.xbutton("Save", self.save)
        h.xpanel()
        self.vbox2.intercept(0)
        self.vbox2.map()
        self.hbox1.intercept(0)
        self.hbox1.map()
        self.update()
        self.region_list.select_action(self.update)
        self.region_list.select("(NEW)")
        self.update()

    def info(self):
        h.xpanel(f"Information on {self.geoselector.text[self.geoselector.option]}")
        option_panel = self.option_panels[self.geoselector.option - 1]
        for line in option_panel.info.split("\n"):
            h.xlabel(line)
        h.xpanel()

    def geo_switched(self):
        # called whenever the geometry has been switched
        geo_type_id = self.geoselector.option - 1
        self.deck.flip_to(geo_type_id)
        region_name = self.region_list.selected()
        region_info = regions.get(region_name, default_region)
        self.option_panels[geo_type_id].set_options(region_info)

    def update(self):
        # update the names
        self.region_list.select_action("")
        selected = self.region_list.selected()
        region_names = sorted(list(regions.keys()), key=lambda s: s.lower())
        region_names = ["(NEW)"] + region_names
        self.region_list.set_list(region_names)
        if selected is not None:
            self.region_list.select(selected)
        self.update_right_panel()
        self.region_list.select_action(self.update)

    def update_right_panel(self):
        region_name = self.region_list.selected()
        if region_name is None or region_name == "(NEW)":
            self.name_editor.text("")
        else:
            self.name_editor.text(region_name)
        region_info = regions.get(region_name, default_region)
        self.nrnregion_selector.select(region_info.get("nrn_region", None))
        self.geoselector.select_by_name(region_info.get("geometry", "Inside"))

    def save(self):
        region_name = self.name_editor.text()
        if not region_name.strip():
            h.continue_dialog("A name is required.")
            return
        if region_name in regions:
            morphology = regions[region_name].get("morphology", "No Sections")
        else:
            morphology = "No Sections"
        regions[region_name] = {
            "morphology": morphology,
            "geometry": self.geoselector.text[self.geoselector.option],
        }
        geo_type_id = self.geoselector.option - 1
        regions[region_name].update(self.option_panels[geo_type_id].get_options())
        regions[region_name]["nrn_region"] = self.nrnregion_selector.selected()
        _the_rxd_builder.update()
        self.region_list.select(region_name)

    def delete(self):
        region_name = self.region_list.selected()
        if region_name in regions:
            del regions[region_name]
            _the_rxd_builder.update()


class InstantiatePane:
    def __init__(self):
        self.vbox = h.VBox(3)
        self.vbox.intercept(True)
        h.xpanel("")
        h.xlabel("You may want to save your work (File -> Save Session) before")
        h.xlabel("instantiating,  as there is currently no way to make changes")
        h.xlabel("after the RxD model has been instantiated.")
        h.xpanel()
        self.spacer = h.TextEditor("", 1, 30)
        self.spacer.readonly(1)
        self.spacer.map()
        h.xpanel("", 1)
        h.xlabel("When you are ready, click:   ")
        h.xbutton("Instantiate", _instantiate)
        h.xpanel()
        self.vbox.intercept(False)
        self.vbox.map()


class _RxDBuilder:
    def __init__(self):
        global rxd_builder_tab

        self.vbox = h.VBox(3)
        self.vbox.intercept(True)

        if rxd_builder_tab not in [1, 2, 3, 4, 5]:
            rxd_builder_tab = 1

        h.xpanel("", 1)
        h.xradiobutton("Regions    ", self.regions, 1 if rxd_builder_tab == 1 else 0)
        h.xradiobutton("Species    ", self.species, 1 if rxd_builder_tab == 2 else 0)
        h.xradiobutton(
            "Reactions    ", self.reactions, 1 if rxd_builder_tab == 3 else 0
        )
        h.xradiobutton(
            "Morphology    ", self.morphology, 1 if rxd_builder_tab == 4 else 0
        )
        h.xradiobutton(
            "Instantiate", self.instantiate, 1 if rxd_builder_tab == 5 else 0
        )
        h.xpanel()

        self.deck = h.Deck()
        self.deck.intercept(True)
        self.regionpane = RegionPane()
        self.speciespane = SpeciesPane()
        self.reactionpane = ReactionPane()
        self.morphologypane = MorphologyPane()
        self.instantiatepane = InstantiatePane()
        self._the_reaction_editor = _ReactionEditor()
        self.deck.intercept(False)
        self.deck.map()

        self.deck.flip_to(rxd_builder_tab - 1)

        self.vbox.save(self.save)

        self.vbox.intercept(False)
        self.vbox.full_request(1)

    @property
    def is_mapped(self):
        return self.vbox.ismapped()

    def map(self, name=None, left=None, top=None, width=None, height=None):
        if self.is_mapped:
            return
        if name is None:
            if rxd_builder_left is None:
                self.vbox.map("RxD Builder")
            else:
                self.vbox.map(
                    "RxD Builder",
                    rxd_builder_left,
                    rxd_builder_top,
                    rxd_builder_width,
                    rxd_builder_height,
                )
        else:
            self.vbox.map(name, left, top, width, height)

    def save(self):
        self.vbox.save('nrnpython("import neuron.rxd.gui")')
        self.vbox.save(f'nrnpython("neuron.rxd.gui.regions = {regions!r}")')
        self.vbox.save(f'nrnpython("neuron.rxd.gui.species = {species!r}")')
        self.vbox.save(f'nrnpython("neuron.rxd.gui.all_reactions = {all_reactions!r}")')
        self.vbox.save(
            f'nrnpython("neuron.rxd.gui.rxd_builder_tab = {rxd_builder_tab!r}")'
        )
        self.vbox.save(
            f'nrnpython("neuron.rxd.gui.has_instantiated = {has_instantiated!r}")'
        )
        self.vbox.save('nrnpython("from neuron import h")')
        self.vbox.save(
            'nrnpython("h.ocbox_ = neuron.rxd.gui.RxDBuilder(visible=False)")'
        )

        if has_instantiated:
            self.vbox.save('nrnpython("h.ocbox_.instantiate()")')

    def regions(self):
        global rxd_builder_tab
        rxd_builder_tab = 1
        self.deck.flip_to(0)

    def species(self):
        global rxd_builder_tab
        rxd_builder_tab = 2
        self.deck.flip_to(1)
        self.vbox.full_request(1)

    def reactions(self):
        global rxd_builder_tab
        rxd_builder_tab = 3
        self.deck.flip_to(2)

    def morphology(self):
        global rxd_builder_tab
        rxd_builder_tab = 4
        self.deck.flip_to(3)
        self.morphologypane.update()

    def instantiate(self):
        global rxd_builder_tab
        rxd_builder_tab = 5
        self.deck.flip_to(4)
        # self.instantiatepane.update()

    def update(self):
        self.morphologypane.update()
        self.regionpane.update()
        self.speciespane.update()
        self._the_reaction_editor.update()


_the_rxd_builder = None


class NrnRegionSelector:
    def __init__(self):
        h.xpanel("NrnRegionSelector", 1)
        h.xlabel("Electrophysiology region: ")
        self.opt1, self.opt2, self.opt3 = 0, 0, 1
        h.xcheckbox("(I)nside  ", (self, "opt1"), self._click_inside)
        h.xcheckbox("(O)utside  ", (self, "opt2"), self._click_outside)
        h.xcheckbox("Neither", (self, "opt3"), self._click_neither)
        h.xpanel()

    def _click_inside(self):
        self.opt1, self.opt2, self.opt3 = 1, 0, 0
        self.region = "i"

    def _click_outside(self):
        self.opt1, self.opt2, self.opt3 = 0, 1, 0
        self.region = "o"

    def _click_neither(self):
        self.opt1, self.opt2, self.opt3 = 0, 0, 1
        self.region = None

    def selected(self):
        return self.region

    def select(self, name):
        if name == "i":
            self._click_inside()
        elif name == "o":
            self._click_outside()
        else:
            self._click_neither()


class SectionListSelector(_PartialSelector):
    def __init__(self):
        self.ell = h.List()
        self.ell.browser("title", "s")
        self.update()
        self.ell.select(-1)

    def update(self, select=None):
        was_selected = self.selected()
        self.ell.remove_all()
        self.names, self.mapping = get_sectionlists()
        self.names = ["No Sections", "All Sections"] + self.names
        for name in self.names:
            self._append(name)
        # restore selection if possible
        if was_selected is None:
            self.ell.select(-1)
        else:
            self.select(was_selected)

    def _append(self, name):
        self.ell.append(h.String(name))


class SpeciesSelectorWithRegions(_PartialSelector):
    def __init__(self, allow_without_region, allow_with_region):
        self.allow_without_region = allow_without_region
        self.allow_with_region = allow_with_region
        self.ell = h.List()
        self.ell.browser("title", "s")
        self.update()

    def update(self):
        self.ell.remove_all()
        self.names = []
        for s in species:
            if self.allow_without_region:
                self.names.append(s)
            if self.allow_with_region:
                for r in species[s]["regions"]:
                    self.names.append(f"{s}[{r}]")
        self.names = sorted(self.names, key=lambda s: s.lower())
        for name in self.names:
            self._append(name)

    def _append(self, name):
        self.ell.append(h.String(name))


class MembraneSelector(_PartialSelector):
    def __init__(self):
        self.ell = h.List()
        self.ell.browser("title", "s")
        self.update()

    def update(self):
        selected = self.selected()
        boundary_types = ("Membrane", "Constant 2D Area/Length")
        self.ell.remove_all()
        self.names = [
            name
            for name, data in zip(list(regions.keys()), list(regions.values()))
            if data["geometry"] in boundary_types
        ]
        self.names = sorted(self.names, key=lambda s: s.lower())
        for name in self.names:
            self._append(name)
        self.select(selected)

    def _append(self, name):
        self.ell.append(h.String(name))


class RegionList(_PartialSelector):
    def __init__(self, title, names=[]):
        self.vbox = h.VBox(3)
        self.vbox.intercept(True)
        if title is not None:
            h.xpanel("regionlist")
            h.xlabel(title)
            h.xpanel()
        self.ell = h.List()
        self.set_list(names)
        self.ell.browser("title", "s")
        self.vbox.intercept(False)
        self.vbox.map()

    def append(self, name):
        self.ell.append(h.String(name))
        self.names.append(name)

    def remove(self, name):
        try:
            i = self.names.index(name)
        except ValueError:
            return
        self.ell.remove(i)
        del self.names[i]

    def set_list(self, names):
        self.names = []
        self.ell.remove_all()
        for name in names:
            self.append(name)


class _MorphologyPane:
    def __init__(self):
        self.vbox = h.VBox(3)
        self.vbox.intercept(True)

        h.xpanel("")
        h.xlabel("Select the morphology corresponding to each region")
        h.xpanel()

        self.hbox = h.HBox(3)
        self.hbox.intercept(True)

        self.region_list = RegionList(None)

        self.sectionlist_selector = SectionListSelector()
        self.sectionlist_selector.select_action(self.change_morphology_association)

        self.shape_plot = h.Shape()
        self.shape_plot.show(1)

        self.hbox.intercept(False)
        self.hbox.map()

        self.vbox.intercept(False)

    def map(self):
        self.vbox.map("Morphology Pane")

    def change_morphology_association(self):
        region_name = self.region_list.selected()
        morph_name = self.sectionlist_selector.selected()
        if region_name in regions:
            regions[region_name]["morphology"] = morph_name
        if morph_name == "No Sections":
            self.shape_plot.color_all(1)
        elif morph_name == "All Sections":
            self.shape_plot.color_all(2)
        else:
            self.shape_plot.color_all(1)
            self.shape_plot.color_list(
                h.SectionList[self.sectionlist_selector.mapping[morph_name]], 2
            )

    def update(self):
        # update the names
        self.region_list.select_action("")
        selected = self.region_list.selected()
        region_names = sorted(list(regions.keys()), key=lambda s: s.lower())
        self.region_list.set_list(region_names)
        if selected is not None:
            self.region_list.select(selected)
        self.update_right_panel()
        self.region_list.select_action(self.update)
        self.sectionlist_selector.select_action("")
        self.sectionlist_selector.update()
        self.sectionlist_selector.select_action(self.change_morphology_association)
        self.change_morphology_association()

    def update_right_panel(self):
        if self.region_list.selected() is None:
            if self.region_list.names:
                self.region_list.select(self.region_list.names[0])
        if self.region_list.selected() is not None:
            region = regions[self.region_list.selected()]
            self.sectionlist_selector.select(region.get("morphology", "No Sections"))
        else:
            self.sectionlist_selector.select("No Sections")


class RegionSelector:
    def __init__(self, name):
        self.name = name
        selected = species[name]["regions"]
        self._setup_left_right(selected)
        self.region_selector = h.HBox(3)
        self.region_selector.intercept(1)
        self.left_list = RegionList("Nonselected Regions")
        self.arrow_col = h.VBox(3)
        self.arrow_col.intercept(1)
        self.arrow_panel = h.xpanel("")
        h.xlabel("")
        h.xlabel("")
        self.right_arrow = h.xbutton(">", self._do_right_arrow)
        self.left_arrow = h.xbutton("<", self._do_left_arrow)
        h.xlabel("")
        h.xlabel("")
        h.xpanel()
        self.arrow_col.intercept(0)
        self.arrow_col.map()
        self.right_list = RegionList("Selected Regions")
        self.region_selector.intercept(0)
        self.region_selector.map()
        self._update_lists()

    def _update_lists(self):
        self.left_list.set_list(self.left_list_items)
        self.right_list.set_list(self.right_list_items)

    def _update_regions(self):
        species[self.name]["regions"] = self.right_list.names

    def _setup_left_right(self, right):
        left = list(regions.keys())
        right = list(right)

        # only keep those things that belong to the full region names
        # but don't put anything kept on the left
        for item in right:
            if item not in left:
                right.remove(item)
            else:
                left.remove(item)

        self.left_list_items = left
        self.right_list_items = right

    def _do_left_arrow(self):
        name = self.right_list.selected()
        if name is None:
            return
        self.right_list.remove(name)
        self.left_list.append(name)
        self._update_regions()

    def _do_right_arrow(self):
        name = self.left_list.selected()
        if name is None:
            return
        self.left_list.remove(name)
        self.right_list.append(name)
        self._update_regions()

    def selected(self):
        return list(self.right_list.names)


class SpeciesLocator:
    def __init__(self, name):
        self.vbox = h.VBox(3)
        self.vbox.intercept(True)
        h.xpanel("")
        h.xlabel(name)
        h.xpanel()
        self.regions_selector = RegionSelector(name)
        self.vbox.intercept(False)
        self.vbox.map()
        self.name = name

    def regions(self):
        return self.regions_selector.selected()


class SpeciesPanel:
    def __init__(self):
        self.vbox = h.VBox(3)
        self.vbox.intercept(True)
        self.species_locs = {}
        for i, name in enumerate(species.keys()):
            if i:
                h.xpanel("")
                h.xlabel("")
                h.xpanel()
            self.species_locs[name] = SpeciesLocator(name)
        self.vbox.intercept(False)
        self.vbox.map()

    def regions(self):
        return dict([(name, loc.regions()) for name, loc in self.species_locs.items()])


class _SpeciesEditor:
    def __init__(self):
        self.hbox = h.HBox(3)
        self.hbox.save("")
        self.hbox.intercept(True)
        self.vbox = h.VBox(3)
        self.vbox.intercept(True)
        self.ell = h.List()
        self._set_list()
        self.ell.browser("", "s")
        self.ell.select(0)
        h.xpanel("")
        h.xbutton("Delete", self._delete)
        h.xpanel()
        self.vbox.intercept(False)
        self.vbox.map()
        self.vbox = h.VBox(3)
        self.vbox.intercept(True)

        h.xpanel("")
        h.xlabel("Name")
        h.xpanel()

        self.vbox.adjuster(15)
        self.name_editor = h.TextEditor("", 1, 30)
        self.name_editor.map()

        self._set_values("", 0, 0)

        h.xpanel("")
        h.xvalue("Charge", (self, "charge"))
        h.xvalue("Diff Const", (self, "d"))
        h.xpanel()

        h.xpanel("", 1)
        h.xbutton("Revert", self._revert)
        h.xbutton("Save", self._save)
        h.xpanel()

        self.vbox.intercept(False)
        self.vbox.map()
        self.hbox.intercept(False)

        self.ell.select_action(self._new_or_display, 1)

    def _new_or_display(self):
        if self.selected() is None:
            self._set_values("", 0, 0)
        else:
            self._revert()

    def _set_values(self, name, charge, diff):
        self.charge = charge
        self.d = diff
        self.name_editor.text(name)

    def _revert(self):
        name = self.selected()
        if name is not None:
            data = species[name]
            self._set_values(name, data["charge"], data["d"])
        else:
            h.continue_dialog("Nothing to revert to")

    def _save(self):
        name = self.name_editor.text()
        if not name:
            h.continue_dialog("Must enter a name to save")
        else:
            if name in species:
                regions = species[name]["regions"]
            else:
                regions = []
            species[name] = {"charge": self.charge, "d": self.d, "regions": regions}
            self._set_list()
            self._select(name)

    def _delete(self):
        name = self.selected()
        if name is not None:
            del species[name]
            self._set_list()
        else:
            h.continue_dialog("Nothing to delete")
        self._new_or_display()

    def _set_list(self, do_update=True):
        self.ell.remove_all()
        self.append("(NEW)")
        self._names = list(species.keys())
        self._names.sort()
        for name in self._names:
            self.append(name)
        if do_update:
            try:
                species_pane_update()
            except NameError:
                pass

    def append(self, name):
        self.ell.append(h.String(name))

    def _select(self, name):
        self.ell.select(1 + self._names.index(name))

    def selected(self):
        item = int(self.ell.selected())
        if item == -1 or item == 0:
            return None
        return self._names[item - 1]

    @property
    def is_mapped(self):
        return self.hbox.ismapped()

    def map(self):
        self.hbox.map("Species Editor")


_the_species_editor = _SpeciesEditor()


# this is a way of faking the Singleton pattern
def SpeciesEditor():
    if not _the_species_editor.is_mapped:
        _the_species_editor.map()
    _the_species_editor._set_list(do_update=False)
    # select (NEW)
    _the_species_editor.ell.select(0)


class _SpeciesPane:
    def __init__(self):
        self.vbox = h.VBox(3)
        self.vbox.intercept(True)
        h.xpanel("")
        h.xlabel("Edit Species and Select Regions for Each Species")
        h.xbutton("Species Editor", SpeciesEditor)
        h.xpanel()
        self.deck = h.Deck()
        self.deck.intercept(True)
        self.speciespanel = SpeciesPanel()
        self.deck.intercept(False)
        self.deck.map()
        self.deck.flip_to(0)
        self.vbox.intercept(False)

    def _update_panel(self):
        self.deck.remove_last()
        self.deck.intercept(True)
        self.speciespanel = SpeciesPanel()
        self.deck.intercept(False)
        self.deck.flip_to(0)

    def update(self):
        self._update_panel()

    @property
    def is_mapped(self):
        return self.vbox.ismapped()

    def map(self):
        self.vbox.map("Species Pane")


_the_species_pane = _SpeciesPane()


# this is a way of faking the Singleton pattern
def SpeciesPane():
    _the_species_pane = _SpeciesPane()
    _the_species_pane.map()
    return _the_species_pane


def species_pane_update():
    global _the_rxd_builder
    #  print 'species_pane_update'
    if _the_rxd_builder:
        deck = _the_rxd_builder.deck
        #    print deck.hname()
        deck.remove(1)
        deck.intercept(1)
        _the_rxd_builder.speciespane = SpeciesPane()
        deck.intercept(0)
        deck.move_last(1)
        deck.flip_to(rxd_builder_tab - 1)


class SpeciesMultiSelector:
    def __init__(self, selector, allow_with_region=False, allow_without_region=True):
        self.selector = selector
        self.vbox = h.VBox(3)
        self.vbox.intercept(True)
        self.ell = h.List()
        self.names = []
        if allow_without_region:
            self.names += list(species.keys())
        if allow_with_region:
            for s in list(species.keys()):
                for r in species[s]["regions"]:
                    self.names.append(f"{s}[{r}]")

        self.names.sort()
        for name in self.names:
            self.ell.append(h.String(name))
        self.ell.browser("title", "s")
        self.data = numpy.zeros(1)
        h.xpanel("")
        h.xpvalue("Multiplicity", neuron.numpy_element_ref(self.data, 0))
        h.xpanel()
        h.xpanel("", 1)
        h.xbutton("Cancel", self._cancel)
        h.xbutton("Accept", self._accept)
        h.xpanel()
        self.vbox.intercept(False)
        self.vbox.map("Species MultiSelector")

    def _accept(self):
        i = int(self.ell.selected())
        mult = self.data[0]
        if i == -1 or mult <= 0:
            h.continue_dialog("Must select a Species with positive multiplicity")
            return
        self.selector.add(self.names[i], mult)
        self.vbox.unmap()

    def _cancel(self):
        self.vbox.unmap()


class LRHSSelector(_PartialSelector):
    def __init__(self, allow_without_region=True, allow_with_region=False):
        self.allow_without_region = allow_without_region
        self.allow_with_region = allow_with_region
        self.vbox = h.VBox(3)
        self.vbox.intercept(True)
        self.names = []
        self.ell = h.List()
        self._update_list()
        self.species = []
        self.mults = []
        self.ell.browser("", "s")
        h.xpanel("", 1)
        h.xbutton("Add", self._add)
        h.xbutton("Remove", self._remove)
        h.xpanel()
        self.vbox.intercept(False)
        self.vbox.map("L/RHS Selector")

    def add(self, name, mult):
        self.species.append(name)
        if mult == int(mult):
            mult = int(mult)
        self.mults.append(mult)
        self.names.append(f"{mult:g} * {name}")
        self._update_list()

    def _add(self):
        SpeciesMultiSelector(
            self,
            allow_with_region=self.allow_with_region,
            allow_without_region=self.allow_without_region,
        )

    def _update_list(self):
        self.ell.remove_all()
        for name in self.names:
            self._append(name)

    def _append(self, name):
        self.ell.append(h.String(name))

    def _remove(self):
        self.remove(self.selected())

    def remove(self, name):
        try:
            i = self.names.index(name)
        except ValueError:
            return
        self.ell.remove(i)
        del self.names[i]
        del self.species[i]
        del self.mults[i]
        self._update_list()

    def update(self, data):
        self.species = []
        self.mults = []
        self.names = []
        if data:
            for name, mult in data:
                self.add(name, mult)
        else:
            self.ell.remove_all()

    def selection(self):
        return [(name, mult) for name, mult in zip(self.names, self.mults)]


class SpecificRateEditor:
    def __init__(self):
        self.vbox = h.VBox(3)
        self.vbox.intercept(True)
        self.selector = SpeciesSelectorWithRegions(True, True)
        h.xpanel("")
        h.xlabel("Production Rate [units: mM/ms]:")
        h.xpanel()
        self.vbox.adjuster(15)
        self.kf_editor = h.TextEditor("0", 1, 30)
        self.kf_editor.map()
        self.vbox.intercept(False)
        self.vbox.map()

    def update(self, data):
        self.kf_editor.text(data["kf"])
        self.selector.update()
        self.selector.select(data.get("species", ""))

    def get_options(self):
        return {
            "type": "rate",
            "kf": self.kf_editor.text(),
            "species": self.selector.selected(),
        }


def _do_nothing():
    pass


class SpecificReactionEditor:
    def __init__(self):
        self.vbox = h.VBox(3)
        self.vbox.intercept(True)
        self.hbox = h.HBox(3)
        self.hbox.intercept(True)
        self.lhs_selector = LRHSSelector()
        h.xpanel("")
        h.xlabel("")
        h.xlabel("")
        h.xbutton("kf -->", _do_nothing)
        h.xbutton("<-- kb", _do_nothing)
        h.xpanel()
        self.rhs_selector = LRHSSelector()
        self.hbox.intercept(False)
        self.hbox.map("Specific Reaction Editor")

        h.xpanel("")
        h.xlabel("kf:")
        h.xpanel()
        self.vbox.adjuster(15)

        self.kf_editor = h.TextEditor("0", 1, 30)
        self.kf_editor.map()

        h.xpanel("")
        h.xlabel("kb:")
        h.xpanel()
        self.vbox.adjuster(15)

        self.kb_editor = h.TextEditor("0", 1, 30)
        self.kb_editor.map()

        self.is_massaction = True
        h.xpanel("")
        h.xcheckbox("Mass Action", (self, "is_massaction"))
        h.xpanel()

        self.vbox.intercept(False)
        self.vbox.map()

    def update(self, data):
        self.is_massaction = data.get("massaction", False)
        self.kf_editor.text(data["kf"])
        self.kb_editor.text(str(data.get("kb", "0")))
        self.lhs_selector.update(data.get("lhs", []))
        self.rhs_selector.update(data.get("rhs", []))

    def get_options(self):
        return {
            "type": "reaction",
            "kf": self.kf_editor.text(),
            "kb": self.kb_editor.text(),
            "lhs": self.lhs_selector.selection(),
            "rhs": self.rhs_selector.selection(),
        }


class SpecificMultiCompartmentReactionEditor:
    def __init__(self):
        self.vbox = h.VBox(3)
        self.vbox.intercept(True)
        self.hbox = h.HBox(3)
        self.hbox.intercept(True)
        self.lhs_selector = LRHSSelector(
            allow_with_region=True, allow_without_region=False
        )
        h.xpanel("")
        h.xlabel("")
        h.xlabel("")
        h.xbutton("kf -->", _do_nothing)
        h.xbutton("<-- kb", _do_nothing)
        h.xpanel()
        self.rhs_selector = LRHSSelector()
        self.hbox.intercept(False)
        self.hbox.map("Specific Reaction Editor")

        h.xpanel("")
        h.xlabel("kf:")
        h.xpanel()

        self.vbox.adjuster(15)
        self.kf_editor = h.TextEditor("0", 1, 30)
        self.kf_editor.map()

        h.xpanel("")
        h.xlabel("kb:")
        h.xpanel()

        self.vbox.adjuster(15)
        self.kb_editor = h.TextEditor("0", 1, 30)
        self.kb_editor.map()

        h.xpanel("")
        h.xlabel("Select the Boundary:")
        h.xpanel()
        self.membrane_selector = MembraneSelector()

        self.is_massaction = True
        self.membrane_current = False
        self.scale_with_area = True
        h.xpanel("")
        h.xcheckbox("Mass Action", (self, "is_massaction"))
        h.xcheckbox("Induces Membrane Current", (self, "membrane_current"))
        h.xcheckbox("Scales With Membrane Area", (self, "scale_with_area"))
        h.xpanel()

        self.vbox.intercept(False)
        self.vbox.map()

    def get_options(self):
        return {
            "type": "multicompartmentreaction",
            "kf": self.kf_editor.text(),
            "kb": self.kb_editor.text(),
            "lhs": self.lhs_selector.selection(),
            "rhs": self.rhs_selector.selection(),
            "membrane": self.membrane_selector.selected(),
        }

    def update(self, data):
        self.membrane_selector.update()
        self.is_massaction = data.get("massaction", False)
        self.kf_editor.text(data["kf"])
        self.kb_editor.text(str(data.get("kb", "0")))
        self.lhs_selector.update(data.get("lhs", []))
        self.rhs_selector.update(data.get("rhs", []))
        self.scale_with_area = data.get("scale_with_area", True)
        self.membrane_current = data.get("membrane_current", False)
        self.membrane_selector.select(data.get("membrane", None))


class ReactionPanelRight:
    def __init__(self):
        self.data = {
            "type": "reaction",
            "kf": "0",
            "kb": "0",
            "massaction": True,
            "lhs": [],
            "rhs": [],
        }
        self.vbox = h.VBox(3)
        self.vbox.intercept(True)
        h.xpanel("")
        h.xlabel("Name:")
        h.xpanel()
        self.vbox.adjuster(15)
        self.name_editor = h.TextEditor("", 1, 30)
        self.name_editor.map()
        self.opt1 = 0
        self.opt2 = 1
        self.opt3 = 0
        h.xpanel("", 1)
        h.xcheckbox("ODE   ", (self, "opt1"), self._select_rate)
        h.xcheckbox("Reaction   ", (self, "opt2"), self._select_reaction)
        h.xcheckbox(
            "MultiCompartmentReaction   ",
            (self, "opt3"),
            self._select_multicompartmentreaction,
        )
        h.xpanel()
        self.deck = h.Deck()
        self.deck.intercept(True)
        self.rate_editor = SpecificRateEditor()
        self.reaction_editor = SpecificReactionEditor()
        self.multicompartmentreaction_editor = SpecificMultiCompartmentReactionEditor()
        self.deck.intercept(False)
        self.deck.map()
        self.deck.flip_to(1)
        self.current_editor = self.reaction_editor
        h.xpanel("", 1)
        h.xbutton("Revert", self._revert)
        h.xbutton("Save", self._save)
        h.xpanel()
        self.vbox.intercept(False)
        self.vbox.map()

    def _save(self):
        name = self.name_editor.text()
        if not name:
            h.continue_dialog("Must enter a name to save")
        else:
            reaction = self.current_editor.get_options()
            if name in all_reactions:
                reaction["active"] = all_reactions[name]["active"]
            else:
                reaction["active"] = False
            all_reactions[name] = reaction
        _the_rxd_builder.update()
        _the_rxd_builder._the_reaction_editor._select(name)
        _the_rxd_builder._the_reaction_editor._update_view()

    def _revert(self):
        _the_rxd_builder._the_reaction_editor._update_view()

    def _select_rate(self):
        self.opt1, self.opt2, self.opt3 = 1, 0, 0
        self.deck.flip_to(0)
        self.rate_editor.update(self.data)
        self.current_editor = self.rate_editor

    def _select_reaction(self):
        self.deck.flip_to(1)
        self.opt1, self.opt2, self.opt3 = 0, 1, 0
        self.reaction_editor.update(self.data)
        self.current_editor = self.reaction_editor

    def _select_multicompartmentreaction(self):
        self.deck.flip_to(2)
        self.opt1, self.opt2, self.opt3 = 0, 0, 1
        self.multicompartmentreaction_editor.update(self.data)
        self.current_editor = self.multicompartmentreaction_editor

    def update(self, name, data):
        if name is None:
            name = ""
        self.name_editor.text(name)
        self.data = data
        kind = data["type"]
        if kind == "rate":
            self._select_rate()
        elif kind == "reaction":
            self._select_reaction()
        elif kind == "multicompartmentreaction":
            self._select_multicompartmentreaction()

    def update2(self):
        self.multicompartmentreaction.update()


class _ReactionPane(object):
    def __init__(self):
        self.vbox = h.VBox(3)
        self.vbox.intercept(True)
        h.xpanel("")
        h.xlabel("Edit and Enable Reactions")
        h.xbutton("Reaction Editor", ReactionEditor)
        h.xpanel()
        self.deck = h.Deck()
        self.deck.intercept(True)
        h.xpanel("")
        h.xpanel()
        self.deck.intercept(False)
        self.deck.map()
        self.deck.flip_to(0)
        self.vbox.intercept(False)
        self._update_panel()

    def _update_panel(self):
        self.deck.remove_last()
        self.deck.intercept(True)
        self.reaction_names = sorted(
            list(all_reactions.keys()), key=lambda s: s.lower()
        )
        for i, name in enumerate(self.reaction_names):
            self.__setattr__(
                f"include_list{i}", 1 if all_reactions[name]["active"] else 0
            )
        h.xpanel("")
        if self.reaction_names:
            for i, name in enumerate(self.reaction_names):
                h.xcheckbox(
                    name, (self, f"include_list{i}"), self._update_active_reactions
                )
        else:
            h.xlabel("*** No reactions defined yet ***")
        h.xpanel()
        self.deck.intercept(False)
        self.deck.flip_to(0)

    def _update_active_reactions(self):
        for i, name in enumerate(self.reaction_names):
            all_reactions[name]["active"] = (
                True if getattr(self, f"include_list{i}") else False
            )

    @property
    def is_mapped(self):
        return self.vbox.ismapped()

    def map(self):
        self.vbox.map("Reaction Pane")


_the_reaction_pane = _ReactionPane()


# this is a way of faking the Singleton pattern
def ReactionPane():
    if not _the_reaction_pane.is_mapped:
        _the_reaction_pane.map()


class _ReactionEditor:
    def __init__(self):
        self.hbox = h.HBox(3)
        self.hbox.save("")
        self.hbox.intercept(True)
        self.vbox = h.VBox(3)
        self.vbox.intercept(True)
        self.ell = h.List()
        self._set_list()
        self.ell.browser("", "s")
        self.ell.select(0)
        h.xpanel("")
        h.xbutton("Delete", self._delete)
        h.xpanel()
        self.vbox.intercept(False)
        self.vbox.map()
        self.right_panel = ReactionPanelRight()
        self.hbox.intercept(False)
        self.ell.select_action(self._update_view)
        self.hbox.full_request(1)

    def update(self):
        self._set_list()
        self._update_view()

    def _update_view(self):
        name = self.selected()
        if name is None:
            data = {
                "type": "reaction",
                "kf": "0",
                "kb": "0",
                "massaction": True,
                "lhs": [],
                "rhs": [],
            }
        else:
            data = all_reactions[name]
        self.right_panel.update(name, data)

    def _select(self, name):
        self.ell.select(1 + self._names.index(name))

    def selected(self):
        item = int(self.ell.selected())
        if item == -1 or item == 0:
            return None
        return self._names[item - 1]

    def _delete(self):
        try:
            del all_reactions[self.selected()]
        except:
            pass
        self._set_list()

    @property
    def is_mapped(self):
        return self.hbox.ismapped()

    def map(self):
        self.hbox.map("Reaction Editor")

    def _set_list(self):
        selected = self.selected()
        self.ell.remove_all()
        self._append("(NEW)")
        self._names = sorted(list(all_reactions.keys()), key=lambda s: s.lower())
        for name in self._names:
            self._append(name)
        try:
            species_pane_update()
        except NameError:
            pass
        try:
            _the_reaction_pane._update_panel()
        except NameError:
            pass
        if selected in self._names:
            self._select(selected)
        else:
            self.ell.select(0)

    def _append(self, name):
        self.ell.append(h.String(name))
