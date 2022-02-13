"""
Some tests of nrnpy_pyCallObject that need to be run manually as they involve
errors generated via gui usage. Run with 'python -i gui_pycallobject.py'
axonerr: ZeroDivisionError: division by zero
axonexit: exits immediately (need 'stty sane' in termina)
In the Graph, select 'mouse events'.
  hold Crtl key and press mouse button: ZeroDivisionError
  hold Shift key and press mouse button: exits immediately (need 'stty sane')

"""

from neuron import h, gui
import sys

soma = h.Section(name="soma")
axon = h.Section(name="axon")
axonerr = h.Section(name="axonerr")
axonexit = h.Section(name="axonexit")


def select(sec):
    print("select: {} {}".format(sec, type(sec)))
    if sec == axonerr:
        print("substantial memory leak, e.g. ivHit, ivEvent")
        print(1 / 0)
    if sec == axonexit:
        print('need "stty sane" on exit')
        sys.exit(0)


def accept(sec):
    print("accept: {}".format(sec))


sb = h.SectionBrowser()
sb.select_action(select)
sb.accept_action(accept)

my_list = h.List()
for word in ["NEURON", "HOC", "Python", "NMODL"]:
    my_list.append(h.String(word))


def label_with_lengths():
    item_id = int(h.hoc_ac_)
    if item_id == 4:
        1 / 0
    if item_id == 5:
        sys.exit(0)
    item = my_list[item_id].s
    return "%s (%d)" % (item, len(item))


my_list.browser("Words!", label_with_lengths)
# first append will generate error but will add item to list
try:
    my_list.append(h.String("error"))
except:
    print("appending 'error' failed")

# second append will exit
my_list.append(h.String("exit"))
print("this does not print if not iteractive.")
assert my_list.count() == 6


def on_event(event_type, x, y, keystate):
    print("{} {} {} {}".format(event_type, x, y, keystate))
    if keystate == 1:
        1 / 0
    if keystate == 2:
        sys.exit(0)


g = h.Graph()
g.menu_tool("mouse events", on_event)
