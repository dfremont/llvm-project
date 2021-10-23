add_llvm_component_library(LLVMGlulxDesc
  GlulxTargetStreamer.cpp
  GlulxMCAsmInfo.cpp
  GlulxMCExpr.cpp
  GlulxMCTargetDesc.cpp
  GlulxInstPrinter.cpp

  LINK_COMPONENTS
  BinaryFormat
  GlulxInfo
  MC
  Support

  ADD_TO_COMPONENT
  Glulx
)
