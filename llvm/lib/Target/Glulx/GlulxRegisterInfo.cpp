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
#include "GlulxInstrInfo.h"
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
  assert(SPAdj == 0);
  MachineInstr &MI = *II;

  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  int64_t FrameOffset = MFI.getObjectOffset(FrameIndex);

  assert(MFI.getObjectSize(FrameIndex) != 0 &&
         "We assume that variable-sized objects have already been lowered, "
         "and don't use FrameIndex operands.");
  Register FrameRegister = getFrameRegister(MF);

  // If this is the address operand of a load or store, make it relative to SP
  // and fold the frame offset directly in.
  unsigned Opcode = MI.getOpcode();
  unsigned Scale = 0;
  switch (Opcode) {
  case Glulx::ASTORE:
  case Glulx::ALOAD:
    Scale = 4;
    break;
  case Glulx::ASTORES:
  case Glulx::ALOADS:
    Scale = 2;
    break;
  case Glulx::ASTOREB:
  case Glulx::ALOADB:
    Scale = 1;
    break;
  }
  unsigned AddrOperandNum = Glulx::getNamedOperandIdx(
      Opcode, Glulx::OpName::addr);
  if (Scale && AddrOperandNum == FIOperandNum) {
    unsigned OffsetOperandNum = Glulx::getNamedOperandIdx(
        Opcode, Glulx::OpName::offset);
    const MachineOperand &Offset = MI.getOperand(OffsetOperandNum);
    if (Offset.isImm()) {
      int64_t TotalOffset = Offset.getImm();
      assert(FrameOffset >= 0 && TotalOffset >= 0);
      if (FrameOffset % Scale == 0 &&
          static_cast<uint64_t>(TotalOffset) <=
              std::numeric_limits<uint32_t>::max()) {
        TotalOffset += FrameOffset / Scale;
        MI.getOperand(OffsetOperandNum).setImm(TotalOffset);
        MI.getOperand(FIOperandNum)
            .ChangeToRegister(FrameRegister, /*isDef=*/false);
        return;
      }
    }
  }

//  // If this is an address being added to a constant, fold the frame offset
//  // into the constant.
//  if (MI.getOpcode() == Glulx::ADD) {
//    MachineOperand &OtherMO = MI.getOperand(3 - FIOperandNum);
//    if (OtherMO.isReg()) {
//      Register OtherMOReg = OtherMO.getReg();
//      if (Register::isVirtualRegister(OtherMOReg)) {
//        MachineInstr *Def = MF.getRegInfo().getUniqueVRegDef(OtherMOReg);
//        // TODO: For now we just opportunistically do this in the case where
//        // the CONST_I32/64 happens to have exactly one def and one use. We
//        // should generalize this to optimize in more cases.
//        if (Def && Def->getOpcode() ==
//                       WebAssemblyFrameLowering::getOpcConst(MF) &&
//            MRI.hasOneNonDBGUse(Def->getOperand(0).getReg())) {
//          MachineOperand &ImmMO = Def->getOperand(1);
//          if (ImmMO.isImm()) {
//            ImmMO.setImm(ImmMO.getImm() + uint32_t(FrameOffset));
//            MI.getOperand(FIOperandNum)
//                .ChangeToRegister(FrameRegister, /*isDef=*/false);
//            return;
//          }
//        }
//      }
//    }
//  }

  // Otherwise, change operand to FP, potentially plus an offset
  const auto *TII = MF.getSubtarget<GlulxSubtarget>().getInstrInfo();
  unsigned FIRegOperand = FrameRegister;  // Default operand will be FP
  if (FrameOffset) {
    // Create "ADD FP, offset" and make it the operand.
    const TargetRegisterClass *PtrRC =
        MRI.getTargetRegisterInfo()->getPointerRegClass(MF);
    FIRegOperand = MRI.createVirtualRegister(PtrRC);
    BuildMI(MBB, *II, II->getDebugLoc(),
            TII->get(Glulx::ADD),
            FIRegOperand)
        .addReg(FrameRegister)
        .addImm(FrameOffset);
  }
  MI.getOperand(FIOperandNum).ChangeToRegister(FIRegOperand, /*isDef=*/false);
}

Register GlulxRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return Glulx::VRFrame;
}

const TargetRegisterClass *
GlulxRegisterInfo::getPointerRegClass(const MachineFunction &MF,
                                      unsigned Kind) const {
  return &Glulx::GPRRegClass;
}
