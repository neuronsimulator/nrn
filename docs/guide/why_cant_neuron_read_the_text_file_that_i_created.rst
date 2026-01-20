.. _why_cant_neuron_read_the_text_file_that_i_created:

Why can't NEURON read the text file (or ``hoc`` file) that I created?
---------------------------------------------------------------------

The Mac, MSWin, and UNIX/Linux versions of NEURON can read ASCII text files created under any of these operating systems. ASCII, which is sometimes called "plain text" or "ANSI text", encodes each character with only 7 bits of each byte. Some text editors offer alternative formats for saving text files, and if you choose one of these you may find that NEURON will not read the file. For example, Notepad under Win2k allows files to be saved as "Unicode text", which will gag NEURON.
