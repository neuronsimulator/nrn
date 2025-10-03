Frequently asked questions (FAQ)
================================

Getting Started & General Usage
===============================

.. toctree::
  :maxdepth: 1

  how_do_i_run_it.rst
  the_best_way_to_learn_how_to_use_neuron.rst
  how_to_get_started_with_neuron.rst
  using_neuron_on_the_mac.rst
  faq_rst2/New_installation_of_NEURON_with_Python.rst

Creating & Using Models
=======================

.. toctree::
  :maxdepth: 1

  how_do_i_create_a_neuron_model.rst
  faq_rst2/Current_Clamp_Electrode_Modeling_with_Electrode_Resistance_and_Capacitance_in_NEURON.rst
  faq_rst2/Extending_a_Single-Compartment_Model_to_a_Multi-Compartment_Model.rst
  faq_rst2/Multicompartment_Soma_Modeling_and_Inhibitory_Synapse_Distribution.rst
  faq_rst2/Collapse_a_dendritic_tree_into_an_arbitrary_number_of_compartments.rst
  faq_rst2/Number_of_Sections_Compartments_and_nseg_in_NEURON.rst
  faq_rst2/Templates_with_a_variable_number_of_sections.rst
  faq_rst2/What_are_single_compartment_neuron_models_useful_for.rst
  faq_rst2/Changing_Channel_Densities_Along_a_Section_Without_Altering_Morphology.rst
  faq_rst2/Is_it_possible_to_isolate_a_section_from_a_neuron_and_reproduce_its_voltage_response_as_if_it_were_still_connected.rst
  faq_rst2/How_are_Sections_Connected_in_NEURON.rst


Mechanisms & NMODL
==================

.. toctree::
  :maxdepth: 1

  mod-file-examples.rst
  compile-mod-files.rst
  mod-files-not-compiling.rst
  incompatible-nmodl-intermediate-files.rst
  nmodl-built-in-functions-list.rst
  hoc-built-in-functions-list.rst
  neuron-built-in-functions-list.rst
  nmodls_built_in_functions.rst
  faq_rst2/Units_Errors_in_mod_Files__How_to_Fix_Common_Issues_in_the_Morgan_Santhakumar_2007_Model.rst
  faq_rst2/Changing_units_in_NMODL_How_do_unit_definitions_affect_current_calculations.rst
  faq_rst2/How_to_change_parameters_used_in_NMODL_for_channel_kinetics_from_hoc_or_Python.rst
  faq_rst2/How_to_access_ionic_currents_defined_in_NMODL_mechanisms_from_HOC_or_Python.rst
  faq_rst2/Noise_MOD_causing_voltage_discontinuities_due_to_random_number_generation_in_BREAKPOINT.rst
  faq_rst2/How_does_a_section_recognize_a_mechanisms_current_in_NEURON.rst
  faq_rst2/Parallel_assignment_of_large_arrays_in_multiple_instances_of_an_NMODL_mechanism.rst
  faq_rst2/What_are_the_units_of_ina_and_ik_in_the_Hodgkin-Huxley_mechanism_in_NEURON.rst
  faq_rst2/How_to_access_and_interpret_3D_morphology_points_after_importing_with_Import3D.rst

Simulation Control & Parameters
===============================

.. toctree::
  :maxdepth: 1

  faq_rst2/Multiple_runs_in_NEURON_produce_different_results__Why_and_how_to_achieve_reproducibility.rst
  faq_rst2/Simulation_with_time_step_greater_than_0_025_ms.rst
  faq_rst2/Variable_Step_Size_and_Integration_Methods_in_NEURON.rst
  faq_rst2/Maximum_dt_in_NEURON_and_PDE_Solver_Methods.rst
  faq_rst2/Factors_Influencing_NEURON_Simulation_Run_Time_Beyond_Total_Compartment_Count.rst
  faq_rst2/Programmatically_Changing_Temperature_During_a_NEURON_Simulation.rst
  faq_rst2/Continue_Simulation_Until_a_Preset_Number_of_Spikes_is_Reached.rst
  faq_rst2/Conditional_termination_of_a_simulation_based_on_spike_times_in_NEURON.rst
  faq_rst2/How_can_I_split_a_NEURON_simulation_into_two_runs_with_parameter_changes_in_between.rst
  faq_rst2/Simulation_with_time_step_greater_than_0_025_ms.rst

Visualization & Output
======================

.. toctree::
  :maxdepth: 1

  faq_rst2/How_do_I_plot_something_other_than_membrane potential.rst
  faq_rst2/How_do_I_save_and_edit_figures.rst
  working_with_postscript_and_idraw.rst
  change-bg-color-neuron.rst
  change-color-scale-shape-plots.rst
  using_plothwat_to_specify_a_variable.rst
  faq_rst2/Recording_the_Second_Derivative_of_an_Action_Potential_in_NEURON.rst

Data Handling & File Operations
===============================

.. toctree::
  :maxdepth: 1

  why_cant_neuron_read_the_text_file_that_i_created.rst
  read-binary-pclamp-file.rst
  using_session_files_for_saving.rst
  faq_rst2/Getting_the_GUI_to_Recognize_Parameters_of_New_Mechanisms.rst
  faq_rst2/How_to_access_and_interpret_3D_morphology_points_after_importing_with_Import3D.rst
  faq_rst2/Finding_the_3D_Coordinates_of_the_Middle_of_a_Section_in_NEURON.rst
  faq_rst2/How_to_access_ionic_currents_defined_in_NMODL_mechanisms_from_HOC_or_Python.rst

Synaptic & Network Modeling
===========================

.. toctree::
  :maxdepth: 1

  drive-postsynaptic-with-spike-times.rst
  faq_rst2/Changing_Synaptic_Conductance_During_a_Sequence_of_Events.rst
  faq_rst2/How_to_activate_a_synapse_using_NetCon_without_a_presynaptic_spike.rst
  netstim_and_random_seed_control.rst
  Inhibition_Synaptic_Weight_How_to_Choose_and_Interpret_It_in_NEURON.rst
  faq_rst2/Difference_between_weight_and_reversal_potential_for_synaptic_connections.rst
  faq_rst2/Tracking_Synaptic_Current_from_a_Specific_Bouton_to_the_Soma_in_NEURON.rst
  faq_rst2/Recording_Synaptic_Currents_to_Approximate_LFP_in_NEURON.rst
  faq_rst2/Vector_play_memory_efficiency_Can_I_apply_scaled_versions_of_one_vector_to_many_synaptic_conductances_without_storing_multiple_copies.rst
  faq_rst2/Play_a_unique_vector_into_a_variable_in_each_segment_of_a_NEURON_model.rst
  faq_rst2/Find_all_point_processes_in_a_NEURON_model.rst
  faq_rst2/Difference_between_weight_and_reversal_potential_for_synaptic_connections.rst
  faq_rst2/How_to_activate_a_synapse_using_NetCon_without_a_presynaptic_spike.rst

Electrophysiology & Stimulation
===============================

.. toctree::
  :maxdepth: 1

  seclamp-vclamp-difference.rst
  seclamp-iclamp-arbitrary-waveform.rst
  current-clamp-pulse-sequence.rst
  current-clamp-event-pulse.rst
  faq_rst2/Ineffective_Simulation_with_Very_Short_Current_Injection_Pulses.rst
  faq_rst2/Action_Potential_Amplitude_Variation_in_Single_Compartment_Models.rst
  faq_rst2/Fast_and_Persistent_Sodium_Currents_in_NEURON.rst
  faq_rst2/Action_Potential_Propagation_and_Intracellular_Voltage_in_NEURON.rst
  faq_rst2/Propagation_Speed_of_Action_Potentials_is_Slower_Than_Expected_in_NEURON_Simulations.rst
  faq_rst2/Direction_of_Current_in_a_Point_Process_in_NEURON.rst
  faq_rst2/Propagation_Speed_of_Action_Potentials_is_Slower_Than_Expected_in_NEURON_Simulations

Calcium & Diffusion Modeling
============================

.. toctree::
  :maxdepth: 1

  faq_rst2/Calcium_Diffusion__Does_the_Outermost_Shell_Represent_Intracellular_Calcium_Concentration.rst
  faq_rst2/Spatial_discretization_for_radial_diffusion_of_calcium.rst
  faq_rst2/Initialization_of_Calcium_Concentration_in_a_Segment_with_Attached_Point_Process.rst
  faq_rst2/Setting_the_Basal_Level_for_Intracellular_Calcium_Concentration_in_NEURON.rst
  faq_rst2/Simple_Internal_Calcium_Concentration_How_to_Correctly_Implement_and_Initialize_Cai_in_NEURON.rst
  faq_rst2/Spatial_discretization_for_radial_diffusion_of_calcium.rst

Technical/Programming in HOC & Python
=====================================

.. toctree::
  :maxdepth: 1

  faq_rst2/What_does_o1_mean_in_hoc_code.rst
  faq_rst2/How_to_assign_multiple_values_to_an_array_in_hoc_and_Python.rst
  faq_rst2/Range_syntax_for_varying_morphology_and_mechanisms_along_a_section_in_Python.rst
  faq_rst2/Arrays_as_RANGE_variables_in_NEURON.rst
  faq_rst2/Parallel_assignment_of_large_arrays_in_multiple_instances_of_an_NMODL_mechanism.rst
  faq_rst2/Dynamically_Creating_and_Managing_Multiple_Vectors_for_APCount_in_NEURON.rst
  tstop_variable_undefined_inside_templates.rst

Performance & Scaling
=====================

.. toctree::
  :maxdepth: 1

  faq_rst2/Load_balancing_with_heterogeneous_morphologies_and_synapses.rst
  faq_rst2/Adding_Many_Cell_Template_Instances_Slows_Down_Simulation_Setup_How_to_Speed_It_Up.rst
  faq_rst2/Does_using_POINTER_in_NET_RECEIVE_significantly_slow_down_simulations_in_NEURON.rst

Miscellaneous / Other Tools
===========================

.. toctree::
  :maxdepth: 1

  how_do_i_print_a_hard_copy_of_a_neuron_window.rst
  can_i_edit_it.rst
  nrn_defaults.rst
  neuron-units.rst
  units_tutorial.rst
  using_the_d_lambda_rule.rst
  finitialize_handler.rst
  exit-neuron-no-gui.rst
  faq_rst2/Interfacing_NEURON_with_other_software_at_runtime.rst
  faq_rst2/Times_of_spikes_vector.rst
  faq_rst2/Fano_Factor_Calculation_for_NetStim_Events_in_NEURON.rst
  faq_rst2/Fitting_Data_Sampled_at_Irregular_Intervals_Using_MRF_in_NEURON.rst
