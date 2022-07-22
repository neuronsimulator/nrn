.. _example_of_generating_movie:

Example of Generating a Movie 
============

`ipynb from google colab <https://colab.research.google.com/drive/1YuDWJt2osDaAqVlMKZ2AZBhqrabWDmjz?usp=sharing#scrollTo=Gn45M0sd_GlD>`_ to google colab

Install NEURON, update matplotlib
----------

The matplotlib update lets us specify ``vertical_axis``

.. code::
    python

    !pip install neuron
    !pip install --upgrade matplotlib

Load the morphology definition 
-----------

.. code::
    python

    !wget https://raw.githubusercontent.com/ramcdougal/dentategranulevideo/master/n275.hoc

Setup the model
--------

.. code::
    python

    from neuron import h
    import matplotlib.pyplot as plt
    import tqdm
    from IPython.display import HTML
    from base64 import b64encode
    from neuron.units import ms, mV

    plt.rcParams["figure.figsize"] = (6, 6)
    h.load_file("stdrun.hoc")
    h.load_file("n275.hoc")

.. code::
    python

    STOP_TIME = 45 * ms
    SAVE_EVERY = 0.25 * ms

.. code::
    python

    for sec in h.allsec():
      sec.nseg = 21
      if 'dend' not in sec.name():
        sec.insert(h.hh)

.. code::
    python

    # setup current pulses to trigger APs
    fire_times = [0 * ms, 15 * ms, 23 * ms, 31 * ms]
    iclamps = []
    for time in fire_times:
        iclamp = h.IClamp(h.soma[0](0.5))
        iclamp.delay = time
        iclamp.amp = 2
        iclamp.dur = 0.5 * ms
        iclamps.append(iclamp)

Simulation control and image saving
------------

.. code::
    python

    def neuron_images():
    # rotation
    ps = h.PlotShape(False)
    ps.plot(plt)
    for theta in range(0, 360, 9):
      plt.gca().view_init(0, theta, vertical_axis="y")
      yield 
    plt.close()
    ps.variable("v")
    ps.scale(-80, 50)
    # now let's run the sim, plt on a new figure every SAVE_EVERY, then yield
    h.finitialize(-65 * mV)
    for i in range(200):
      h.continuerun(i * SAVE_EVERY)
      ps.plot(plt)
      if i < 40:
         theta = 9 * i
      else:
        theta = 0
      plt.gca().view_init(0, theta, vertical_axis="y")
      yield
      plt.close()

.. code::
    python

    def save_all_images():
      for i, _ in tqdm.tqdm(enumerate(neuron_images())):
        plt.savefig(f"{i:04d}.png")

This counts to 40 + 200 = 240. Runs in about twelve minutes. The first 40 is relatively fast because it's only rotating an existing plot, not simulating or making a new plot.

.. code::
    python

    save_all_images()

Put it all together into an MP4
------------

.. code::
    python

    !ffmpeg -r 20 -i %04d.png -vcodec libx264 -crf 25 -pix_fmt yuv420p neuron_movie.mp4

Let's look at the MP4
-------------

.. code::
    python

    # adapted from https://stackoverflow.com/questions/57377185/how-play-mp4-video-in-google-colab
    def show_video(video_path):
      with open(video_path, "r+b") as f:
        video_url = f"data:video/mp4;base64,{b64encode(f.read()).decode()}"
      return HTML(f"<video width=640 controls><source src='{video_url}'></video>")

.. code::
    python

    show_video("neuron_movie.mp4")

