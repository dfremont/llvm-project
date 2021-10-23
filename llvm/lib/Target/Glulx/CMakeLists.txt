add_llvm_component_group(Glulx)

set(LLVM_TARGET_DEFINITIONS Glulx.td)

tablegen(LLVM GlulxGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM GlulxGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM GlulxGenCodeEmitter.inc -gen-emitter)
tablegen(LLVM GlulxGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM GlulxGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM GlulxGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM GlulxGenSubtargetInfo.inc -gen-subtarget)
tablegen(LLVM GlulxGenMCPseudoLowering.inc -gen-pseudo-lowering)

# GlulxCommonTableGen must be defined
add_public_tablegen_target(GlulxCommonTableGen)

add_llvm_target(GlulxCodeGen
  GlulxExplicitLocals.cpp
  GlulxFoldStores.cpp
  GlulxMachineFunctionInfo.cpp
  GlulxOptimizeLiveIntervals.cpp
  GlulxPrepareForLiveIntervals.cpp
  GlulxRegColoring.cpp
  GlulxArgumentMove.cpp
  GlulxUnsignedToSignedDivision.cpp
  GlulxAsmPrinter.cpp
  GlulxFrameLowering.cpp
  GlulxInstrInfo.cpp
  GlulxISelLowering.cpp
  GlulxRegisterInfo.cpp
  GlulxSubtarget.cpp
  GlulxTargetObjectFile.cpp
  GlulxSelectionDAGInfo.cpp
  GlulxISelDAGToDAG.cpp
  GlulxTargetMachine.cpp

  DEPENDS
  intrinsics_gen

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  BinaryFormat
  CodeGen
  Core
  MC
  GlulxDesc
  GlulxInfo
  Scalar
  SelectionDAG
  Support
  Target
  TransformUtils

  ADD_TO_COMPONENT
  Glulx
)

add_subdirectory(TargetInfo)
add_subdirectory(MCAsmStreamer)
add_subdirectory(MCTargetDesc)
