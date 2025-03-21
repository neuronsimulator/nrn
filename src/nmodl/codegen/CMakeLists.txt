# =============================================================================
# Codegen library
# =============================================================================
add_library(
  nmodl_codegen STATIC
  codegen_acc_visitor.cpp
  codegen_transform_visitor.cpp
  codegen_coreneuron_cpp_visitor.cpp
  codegen_neuron_cpp_visitor.cpp
  codegen_cpp_visitor.cpp
  codegen_compatibility_visitor.cpp
  codegen_helper_visitor.cpp
  codegen_info.cpp
  codegen_utils.cpp)
add_dependencies(nmodl_codegen lexer util visitor)
target_link_libraries(nmodl_codegen PRIVATE util)
set_target_properties(nmodl_codegen PROPERTIES OUTPUT_NAME codegen)
