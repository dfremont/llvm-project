//===-- GlulxRegisterInfo.h - Glulx Register Information Impl -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Glulx implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Glulx_GlulxREGISTERINFO_H
#define LLVM_LIB_TARGET_Glulx_GlulxREGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "GlulxGenRegisterInfo.inc"

namespace llvm {
class GlulxSubtarget;

class GlulxRegisterInfo : public GlulxGenRegisterInfo {
protected:
  const GlulxSubtarget &Subtarget;

public:
  GlulxRegisterInfo(const GlulxSubtarget &Subtarget);

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;
  BitVector getReservedRegs(const MachineFunction &MF) const override;
  void eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;

  Register getFrameRegister(const MachineFunction &MF) const override;

  const TargetRegisterClass *
  getPointerRegClass(const MachineFunction &MF,
                     unsigned Kind = 0) const override;
  // This does not apply to Glulx.
  const uint32_t *getNoPreservedMask() const override { return nullptr; }
};

} // end namespace llvm

#endif // end LLVM_LIB_TARGET_Glulx_GlulxREGISTERINFO_H
