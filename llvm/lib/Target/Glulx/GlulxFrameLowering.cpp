//===-- GlulxFrameLowering.cpp - Glulx Frame Information ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the GlulxTargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#include "GlulxFrameLowering.h"
#include "GlulxMachineFunctionInfo.h"
#include "GlulxSubtarget.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

// hasFP - Return true if the specified function should have a dedicated frame
// pointer register. Since we cannot take the address of anything on the Glulx
// stack, we need a pointer into the heap for any nontrivial use of the stack.
bool GlulxFrameLowering::hasFP(const MachineFunction &MF) const {
  return true;
}

MachineBasicBlock::iterator GlulxFrameLowering::eliminateCallFramePseudoInstr(
                                        MachineFunction &MF,
                                        MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator I) const {
  return MBB.erase(I);
}

void GlulxFrameLowering::emitPrologue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  auto &MFI = MF.getFrameInfo();
  assert(MFI.getCalleeSavedInfo().empty() &&
         "Glulx should not have callee-saved registers");

  if (!MFI.hasStackObjects())
    return;

  uint64_t StackSize = MFI.getStackSize();

  auto &ST = MF.getSubtarget<GlulxSubtarget>();
  const auto *TII = ST.getInstrInfo();

  auto InsertPt = MBB.begin();
  while (InsertPt != MBB.end() && Glulx::isArgument(InsertPt->getOpcode()))
    ++InsertPt;
  DebugLoc DL;

  // Work out choice of SP and FP "registers".
  unsigned SPReg = Glulx::VRStack;
  unsigned FPReg = Glulx::VRFrame;
  Align Alignment = MFI.getMaxAlign();
  if (Log2(Alignment) == 0) {
    // No alignment needed; use FP as SP to save one local.
    SPReg = Glulx::VRFrame;
  }

  // Allocate space for the stack.
  BuildMI(MBB, InsertPt, DL, TII->get(Glulx::MALLOC_i), SPReg)
    .addImm(StackSize > 0 ? StackSize : 4);

  // Enforce call frame alignment required by objects on the stack.
  if (Log2(Alignment) > 0) {
    BuildMI(MBB, InsertPt, DL, TII->get(Glulx::ADD), FPReg)
        .addReg(SPReg)
        .addImm(Alignment.value() - 1);
    BuildMI(MBB, InsertPt, DL, TII->get(Glulx::AND), FPReg)
        .addReg(FPReg)
        .addImm((int64_t) ~(Alignment.value() - 1));
  }
}

void GlulxFrameLowering::emitEpilogue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  auto &MFI = MF.getFrameInfo();
  if (!MFI.hasStackObjects())
    return;

  auto &ST = MF.getSubtarget<GlulxSubtarget>();
  const auto *TII = ST.getInstrInfo();
  auto InsertPt = MBB.getFirstTerminator();
  DebugLoc DL;

  if (InsertPt != MBB.end())
    DL = InsertPt->getDebugLoc();

  // Work out choice of SP "register" (see comment in emitPrologue).
  unsigned SPReg = Glulx::VRStack;
  Align Alignment = MFI.getMaxAlign();
  if (Log2(Alignment) == 0)
    SPReg = Glulx::VRFrame;

  // Free stack memory.
  BuildMI(MBB, InsertPt, DL, TII->get(Glulx::MFREE_r))
    .addReg(SPReg);
}

StackOffset
GlulxFrameLowering::getFrameIndexReference(const MachineFunction &MF, int FI,
                                           Register &FrameReg) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  FrameReg = Glulx::VRFrame;
  return StackOffset::getFixed(MFI.getObjectOffset(FI));
}

bool
GlulxFrameLowering::hasReservedCallFrame(const MachineFunction &MF) const {
  return true;
}
