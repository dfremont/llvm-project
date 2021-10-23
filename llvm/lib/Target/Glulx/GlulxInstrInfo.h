//===-- GlulxInstrInfo.h - Glulx Instruction Information ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Glulx implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Glulx_GlulxINSTRINFO_H
#define LLVM_LIB_TARGET_Glulx_GlulxINSTRINFO_H

#include "Glulx.h"
#include "GlulxRegisterInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "GlulxGenInstrInfo.inc"

#define GET_INSTRINFO_OPERAND_ENUM
#include "GlulxGenInstrInfo.inc"

namespace llvm {

namespace Glulx {

int16_t getNamedOperandIdx(uint16_t Opcode, uint16_t NamedIndex);

} // Glulx

class GlulxInstrInfo : public GlulxGenInstrInfo {
public:
  explicit GlulxInstrInfo(const GlulxSubtarget &STI);

  std::pair<unsigned, unsigned>
  decomposeMachineOperandsTargetFlags(unsigned TF) const override;
  ArrayRef<std::pair<unsigned, const char *>>
  getSerializableDirectMachineOperandTargetFlags() const override;

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                   const DebugLoc &DL, MCRegister DstReg, MCRegister SrcReg,
                   bool KillSrc) const override;
  bool analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                     MachineBasicBlock *&FBB,
                     SmallVectorImpl<MachineOperand> &Cond,
                     bool AllowModify) const override;
  unsigned removeBranch(MachineBasicBlock &MBB,
                        int *BytesRemoved = nullptr) const override;
  unsigned insertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                        MachineBasicBlock *FBB, ArrayRef<MachineOperand> Cond,
                        const DebugLoc &DL,
                        int *BytesAdded = nullptr) const override;

  MachineInstr *foldMemoryOperandImpl(
      MachineFunction &MF, MachineInstr &MI, ArrayRef<unsigned> Ops,
      MachineBasicBlock::iterator InsertPt, MachineInstr &LoadMI,
      LiveIntervals *LIS = nullptr) const override;
  MachineInstr *optimizeLoadInstr(MachineInstr &MI,
                                  const MachineRegisterInfo *MRI,
                                  Register &FoldAsLoadDefReg,
                                  MachineInstr *&DefMI) const override;

protected:
  const GlulxSubtarget &Subtarget;
};
}

#endif // end LLVM_LIB_TARGET_Glulx_GlulxINSTRINFO_H
