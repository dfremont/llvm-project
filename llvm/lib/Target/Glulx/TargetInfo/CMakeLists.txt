include_directories( ${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/.. )

add_llvm_component_library(LLVMGlulxInfo
  GlulxTargetInfo.cpp

  LINK_COMPONENTS
  GlulxAsmStreamer
  MC
  Support

  ADD_TO_COMPONENT
  Glulx
)

add_dependencies(LLVMGlulxInfo GlulxCommonTableGen)