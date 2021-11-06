//===-- GlulxInstrInfo.cpp - Glulx Instruction Information ----------------===//
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

#include "GlulxInstrInfo.h"

#include "GlulxTargetMachine.h"
#include "GlulxMachineFunctionInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "Glulx-instrinfo"

#define GET_INSTRINFO_CTOR_DTOR
#include "GlulxGenInstrInfo.inc"

// defines Glulx::getNamedOperandIdx
#define GET_INSTRINFO_NAMED_OPS
#include "GlulxGenInstrInfo.inc"

GlulxInstrInfo::GlulxInstrInfo(const GlulxSubtarget &STI)
    : GlulxGenInstrInfo(),
      Subtarget(STI)
{
}

std::pair<unsigned, unsigned>
GlulxInstrInfo::decomposeMachineOperandsTargetFlags(unsigned TF) const {
  return std::make_pair(TF, 0u);
}

ArrayRef<std::pair<unsigned, const char *>>
GlulxInstrInfo::getSerializableDirectMachineOperandTargetFlags() const {
  using namespace GlulxII;
  static const std::pair<unsigned, const char *> TargetFlags[] = {
      {MO_DEREFERENCE, "glulx-deref"},
      {MO_NO_FLAG, "glulx-nf"}};
  return makeArrayRef(TargetFlags);
}

void GlulxInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MBBI,
                                 const DebugLoc &DL, MCRegister DstReg,
                                 MCRegister SrcReg, bool KillSrc) const {
    BuildMI(MBB, MBBI, DL, get(Glulx::copy_rr), DstReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
    return;
}

bool GlulxInstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                 MachineBasicBlock *&TBB,
                                 MachineBasicBlock *&FBB,
                                 SmallVectorImpl<MachineOperand> &Cond,
                                 bool AllowModify) const {
  // Start from the bottom of the block and work up, examining the
  // terminator instructions.
  MachineBasicBlock::iterator I = MBB.end();
  while (I != MBB.begin()) {
    --I;
    if (I->isDebugInstr())
      continue;

    // Working from the bottom, when we see a non-terminator
    // instruction, we're done.
    if (!isUnpredicatedTerminator(*I))
      break;

    // A terminator that isn't a branch can't easily be handled
    // by this analysis.
    if (!I->isBranch())
      return true;

    // Handle unconditional branches.
    if (I->getOpcode() == Glulx::JUMP) {
      if (!AllowModify) {
        TBB = I->getOperand(0).getMBB();
        continue;
      }

      // If the block has any instructions after a J, delete them.
      while (std::next(I) != MBB.end())
        std::next(I)->eraseFromParent();
      Cond.clear();
      FBB = nullptr;

      // Delete the J if it's equivalent to a fall-through.
      if (MBB.isLayoutSuccessor(I->getOperand(0).getMBB())) {
        TBB = nullptr;
        I->eraseFromParent();
        I = MBB.end();
        continue;
      }

      // TBB is used to indicate the unconditional destination.
      TBB = I->getOperand(0).getMBB();
      continue;
    }
    // Cannot handle conditional branches
    return true;
  }

  return false;
}

unsigned GlulxInstrInfo::insertBranch(MachineBasicBlock &MBB,
                                    MachineBasicBlock *TBB,
                                    MachineBasicBlock *FBB,
                                    ArrayRef<MachineOperand> Cond,
                                    const DebugLoc &DL,
                                    int *BytesAdded) const {
  assert(!BytesAdded && "code size not handled");

  // Shouldn't be a fall through.
  assert(TBB && "insertBranch must not be told to insert a fallthrough");

  if (Cond.empty()) {
    // Unconditional branch
    assert(!FBB && "Unconditional branch with multiple successors!");
    BuildMI(&MBB, DL, get(Glulx::JUMP)).addMBB(TBB);
    return 1;
  }

  llvm_unreachable("Unexpected conditional branch");
}

unsigned GlulxInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                    int *BytesRemoved) const {
  assert(!BytesRemoved && "code size not handled");

  MachineBasicBlock::iterator I = MBB.end();
  unsigned Count = 0;

  while (I != MBB.begin()) {
    --I;
    if (I->isDebugInstr())
      continue;
    if (I->getOpcode() != Glulx::JUMP)
      break;
    // Remove the branch.
    I->eraseFromParent();
    I = MBB.end();
    ++Count;
  }

  return Count;
}

MachineInstr *GlulxInstrInfo::foldMemoryOperandImpl(
    MachineFunction &MF, MachineInstr &MI, ArrayRef<unsigned> Ops,
    MachineBasicBlock::iterator InsertPt, MachineInstr &LoadMI,
    LiveIntervals *LIS) const {
  // We only handle folding of the copy instruction, for now.
  assert(LoadMI.getOpcode() == Glulx::copy_mr);

  // Some optimization passes assume COPY has only reg operands.
  if (MI.getOpcode() == Glulx::COPY)
    return nullptr;

  // Build new instruction with folded-in operand(s).
  MachineInstr *NewMI =
      MF.CreateMachineInstr(get(MI.getOpcode()), MI.getDebugLoc(), true);
  MachineInstrBuilder MIB(MF, NewMI);
  for (unsigned i = 0, e = MI.getNumOperands(); i != e; ++i) {
    MachineOperand &MO = MI.getOperand(i);
    bool Found = false;
    for (unsigned j = 0, f = Ops.size(); j != f; ++j) {
      if (i == Ops[j]) {
        assert(MO.isReg() && "Expected to fold into reg operand!");
        auto &OldOp = LoadMI.getOperand(1);
        OldOp.setTargetFlags(GlulxII::MO_DEREFERENCE);
        MIB.add(OldOp);
        Found = true;
        break;
      }
    }
    if (!Found)
      MIB.add(MO);
  }

  // Insert the new instruction at the specified location.
  MachineBasicBlock *MBB = InsertPt->getParent();
  MBB->insert(InsertPt, NewMI);

  return NewMI;
}

MachineInstr *GlulxInstrInfo::optimizeLoadInstr(MachineInstr &MI,
                                                const MachineRegisterInfo *MRI,
                                                Register &FoldAsLoadDefReg,
                                                MachineInstr *&DefMI) const {
  // Check whether we can move DefMI here.
  DefMI = MRI->getVRegDef(FoldAsLoadDefReg);
  assert(DefMI);
  bool SawStore = false;  // intervening stores are checked in PeepholeOptimizer
  if (!DefMI->isSafeToMove(nullptr, SawStore))
    return nullptr;

  // Collect information about virtual register operands of MI.
  SmallVector<unsigned, 1> SrcOperandIds;
  for (unsigned i = 0, e = MI.getNumOperands(); i != e; ++i) {
    MachineOperand &MO = MI.getOperand(i);
    if (!MO.isReg())
      continue;
    Register Reg = MO.getReg();
    if (Reg != FoldAsLoadDefReg)
      continue;
    if (!MO.isDef())
      SrcOperandIds.push_back(i);
  }
  if (SrcOperandIds.empty())
    return nullptr;

  // Check whether we can fold the def into SrcOperandId.
  if (MachineInstr *FoldMI = foldMemoryOperand(MI, SrcOperandIds, *DefMI)) {
    FoldAsLoadDefReg = 0;
    return FoldMI;
  }

  return nullptr;
}
