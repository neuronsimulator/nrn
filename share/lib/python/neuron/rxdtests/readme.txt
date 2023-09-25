NEURON RxD Test Framework

Robert A McDougal
June 23, 2014

run_all.py -- runs each test in tests/, stores data from the first few timesteps
              in test_data/ and compares it with corresponding data from
              correct_data/


Tests (in tests/ subdirectory):

    pure_diffusion.py -- pure diffusion from a pulse with the fixed step method
    pure_diffusion_cvode.py -- pure diffusion from a pulse with cvode
    cabuf.py	-- ca+buf->cabuf + diffusion of ca on a 2 section cell

              
Notes:
              
    Tests that fail may be run manually by the user to generate graphs that can
    be used to assess the error.

    Each test should run quickly. Only the first few timepoints will be
    compared, so there can be no long initialization delay.

    The files in tests/ could be considered examples of rxd usage, although no
    guarantees are made about the suitability of these files for self-learning.
    
    For more traditional Reaction-Diffusion tutorials see
        https://nrn.readthedocs.io/en/latest/rxd-tutorials/index.html
