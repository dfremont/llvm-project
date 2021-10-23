add_llvm_component_library(LLVMGlulxAsmStreamer
  GlulxMCAsmStreamer.cpp

  LINK_COMPONENTS
  BinaryFormat
  MC
  Support

  ADD_TO_COMPONENT
  Glulx
)
