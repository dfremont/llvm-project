//===-- GlulxTargetMachine.cpp - Define TargetMachine for Glulx -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Implements the info about Glulx target spec.
//
//===----------------------------------------------------------------------===//

#include "GlulxTargetMachine.h"
#include "GlulxISelDAGToDAG.h"
#include "GlulxSubtarget.h"
#include "GlulxTargetObjectFile.h"
#include "TargetInfo/GlulxTargetInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Transforms/Utils.h"

using namespace llvm;

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeGlulxTarget() {
  // Register the target.
  RegisterTargetMachine<GlulxTargetMachine> X(getTheGlulxTarget());

  // Register backend passes
  auto &PR = *PassRegistry::getPassRegistry();
  initializeUnsignedToSignedDivisionPass(PR);
  initializeGlulxArgumentMovePass(PR);
  initializeGlulxFoldStoresPass(PR);
  initializeGlulxPrepareForLiveIntervalsPass(PR);
  initializeGlulxOptimizeLiveIntervalsPass(PR);
  initializeGlulxRegColoringPass(PR);
  initializeGlulxExplicitLocalsPass(PR);
}

static std::string computeDataLayout() {
  std::string Ret = "";

  // Big endian
  Ret += "E";

  // ELF name mangling
  Ret += "-m:e";

  // 32-bit pointers, 8-bit aligned
  Ret += "-p:32:8";

  // 32-bit integers, 8 bit aligned
  Ret += "-i32:8";

  // 32-bit native integer width i.e register are 32-bit
  Ret += "-n32";

  // 32-bit floating point, 8 bit aligned
  Ret += "-f32:8";

  // 32-bit natural stack alignment
  Ret += "-S32";

  return Ret;
}

static Reloc::Model getEffectiveRelocModel(Optional<CodeModel::Model> CM,
                                           Optional<Reloc::Model> RM) {
  return Reloc::Static;
}

GlulxTargetMachine::GlulxTargetMachine(const Target &T, const Triple &TT,
                                       StringRef CPU, StringRef FS,
                                       const TargetOptions &Options,
                                       Optional<Reloc::Model> RM,
                                       Optional<CodeModel::Model> CM,
                                       CodeGenOpt::Level OL,
                                       bool JIT)
    : LLVMTargetMachine(T, computeDataLayout(), TT, CPU, FS, Options,
                        getEffectiveRelocModel(CM, RM),
                        getEffectiveCodeModel(CM, CodeModel::Small), OL),
      TLOF(std::make_unique<GlulxTargetObjectFile>()) {
  // initAsmInfo will display features by llc -march=Glulx on 3.7
  initAsmInfo();
}

const GlulxSubtarget *
GlulxTargetMachine::getSubtargetImpl(const Function &F) const {
  Attribute CPUAttr = F.getFnAttribute("target-cpu");
  Attribute FSAttr = F.getFnAttribute("target-features");

  std::string CPU = !CPUAttr.hasAttribute(Attribute::None)
                        ? CPUAttr.getValueAsString().str()
                        : TargetCPU;
  std::string FS = !FSAttr.hasAttribute(Attribute::None)
                       ? FSAttr.getValueAsString().str()
                       : TargetFS;

  auto &I = SubtargetMap[CPU + FS];
  if (!I) {
    // This needs to be done before we create a new subtarget since any
    // creation will depend on the TM and the code generation flags on the
    // function that reside in TargetOptions.
    resetTargetOptions(F);
    I = std::make_unique<GlulxSubtarget>(TargetTriple, CPU, CPU, FS, *this);
  }
  return I.get();
}

namespace {
class GlulxPassConfig : public TargetPassConfig {
public:
  GlulxPassConfig(GlulxTargetMachine &TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  GlulxTargetMachine &getGlulxTargetMachine() const {
    return getTM<GlulxTargetMachine>();
  }

  FunctionPass *createTargetRegisterAllocator(bool) override;
  void addIRPasses() override;
  bool addInstSelector() override;
  void addPreRegAlloc() override;
  void addPostRegAlloc() override;
  bool addGCPasses() override { return false; }
  void addPreEmitPass() override;

  // No reg alloc
  bool addRegAssignAndRewriteFast() override { return false; }

  // No reg alloc
  bool addRegAssignAndRewriteOptimized() override { return false; }
};
}

TargetPassConfig *GlulxTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new GlulxPassConfig(*this, PM);
}

FunctionPass *GlulxPassConfig::createTargetRegisterAllocator(bool) {
  return nullptr; // No reg alloc
}

void GlulxPassConfig::addIRPasses() {
  addPass(createLowerInvokePass());
  // The lower invoke pass may create unreachable code. Remove it in order not
  // to process dead blocks in setjmp/longjmp handling.
  addPass(createUnreachableBlockEliminationPass());

  TargetPassConfig::addIRPasses();

  addPass(createUnsignedToSignedDivisionPass());
}

// Install an instruction selector pass using
// the ISelDag to gen Glulx code.
bool GlulxPassConfig::addInstSelector() {
  addPass(new GlulxDAGToDAGISel(getGlulxTargetMachine(), getOptLevel()));

  // Run the argument-move pass immediately after the ScheduleDAG scheduler
  // so that we can fix up the ARGUMENT instructions before anything else
  // sees them in the wrong place.
  addPass(createGlulxArgumentMove());

  return false;
}

void GlulxPassConfig::addPreRegAlloc() {
  if (getOptLevel() != CodeGenOpt::None) {
    // Fold stores to constant addresses into indirect store operands.
    addPass(createGlulxFoldStores());
  }
}

void GlulxPassConfig::addPostRegAlloc() {
  // TODO: The following CodeGen passes don't currently support code containing
  // virtual registers. Consider removing their restrictions and re-enabling
  // them.

  // These functions all require the NoVRegs property.
  disablePass(&MachineCopyPropagationID);
  disablePass(&PostRAMachineSinkingID);
  disablePass(&PostRASchedulerID);
  disablePass(&FuncletLayoutID);
  disablePass(&StackMapLivenessID);
  disablePass(&LiveDebugValuesID);
  disablePass(&PatchableFunctionID);
  disablePass(&ShrinkWrapID);

  TargetPassConfig::addPostRegAlloc();
}

void GlulxPassConfig::addPreEmitPass() {
  TargetPassConfig::addPreEmitPass();

  // Preparations and optimizations related to register coloring.
  if (getOptLevel() != CodeGenOpt::None) {
    // LiveIntervals isn't commonly run this late. Re-establish preconditions.
    addPass(createGlulxPrepareForLiveIntervals());

    // Depend on LiveIntervals and perform some optimizations on it.
    addPass(createGlulxOptimizeLiveIntervals());

    // Run the register coloring pass to reduce the total number of registers.
    addPass(createGlulxRegColoring());
  }

  // Convert ARGUMENT instructions to local definitions.
  addPass(createGlulxExplicitLocals());

//  // Perform the very last peephole optimizations on the code.
//  if (getOptLevel() != CodeGenOpt::None)
//    addPass(createWebAssemblyPeephole());
//
//  // Collect information to prepare for MC lowering / asm printing.
//  addPass(createWebAssemblyMCLowerPrePass());
}
