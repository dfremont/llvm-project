//===-- GlulxFrameLowering.h - Define frame lowering for Glulx ----*- C++ -*-===//
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

#ifndef LLVM_LIB_TARGET_Glulx_GlulxFRAMELOWERING_H
#define LLVM_LIB_TARGET_Glulx_GlulxFRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {
  class GlulxSubtarget;

class GlulxFrameLowering : public TargetFrameLowering {
protected:
  const GlulxSubtarget &STI;

public:
  explicit GlulxFrameLowering(const GlulxSubtarget &STI)
    : TargetFrameLowering(TargetFrameLowering::StackGrowsUp,
                          /*StackAlignment*/Align(1),
                          /*LocalAreaOffset*/0,
                          /*TransAl*/Align(1)),
      STI(STI) {}

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  StackOffset getFrameIndexReference(const MachineFunction &MF, int FI,
                                     Register &FrameReg) const override;

  bool hasReservedCallFrame(const MachineFunction &MF) const override;
  MachineBasicBlock::iterator
  eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator I) const override;

  bool hasFP(const MachineFunction &MF) const override;
};
} // end llvm namespace

#endif // end LLVM_LIB_TARGET_Glulx_GlulxFRAMELOWERING_H
