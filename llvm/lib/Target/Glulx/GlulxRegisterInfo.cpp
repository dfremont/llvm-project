//===-- GlulxRegisterInfo.cpp - Glulx Register Information ----------------===//
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

#include "GlulxRegisterInfo.h"
#include "GlulxSubtarget.h"
#include "llvm/Support/Debug.h"

#define GET_REGINFO_TARGET_DESC
#include "GlulxGenRegisterInfo.inc"

#define DEBUG_TYPE "Glulx-reginfo"

using namespace llvm;

GlulxRegisterInfo::GlulxRegisterInfo(const GlulxSubtarget &ST)
  : GlulxGenRegisterInfo(0), Subtarget(ST) {}

const MCPhysReg *
GlulxRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  static const MCPhysReg CalleeSavedRegs[] = {0};
  return CalleeSavedRegs;
}

BitVector GlulxRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  return Reserved;
}

void GlulxRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                           int SPAdj,
                                           unsigned FIOperandNum,
                                           RegScavenger *RS) const {
  llvm_unreachable("Unsupported eliminateFrameIndex");
}

Register GlulxRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  llvm_unreachable("Unsupported getFrameRegister");
}

const TargetRegisterClass *
GlulxRegisterInfo::getPointerRegClass(const MachineFunction &MF,
                                      unsigned Kind) const {
  return &Glulx::I32RegClass;
}
