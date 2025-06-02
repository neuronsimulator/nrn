"""reads and plots data from a file compatible with
stdgui.hoc's clipboard_retrieve()
"""

from neuron import n, gui

n.clipboard_retrieve()  # user selects file to be read, results in n.hoc_obj_[1] and [0]
xvec = n.hoc_obj_[1]
yvec = n.hoc_obj_[0]

if xvec.size() <= 1:
    print("insufficient data")
    n.quit()

# now xvec and yvec contain the xy coords of the points

# up to this point, everything has been generic
# below this point, everything is customized by the user

from plotfi import plotfi

# creates a Graph with customized label and axes
fig = plotfi(xvec, yvec)
