//===-- GlulxFoldStores.cpp - Fold stores into operands -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Folds copy instructions using fixed addresses into store operands.
///
/// This is based on the load folding procedure in PeepholeOptimizer.
///
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/GlulxMCTargetDesc.h"
#include "Glulx.h"
#include "GlulxMachineFunctionInfo.h"
#include "GlulxSubtarget.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define DEBUG_TYPE "glulx-fold-stores"

STATISTIC(NumStoreFold, "Number of stores folded");

namespace {
class GlulxFoldStores final : public MachineFunctionPass {
  MachineRegisterInfo *MRI;

  StringRef getPassName() const override {
    return "Glulx Fold Stores";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

public:
  static char ID; // Pass identification, replacement for typeid
  GlulxFoldStores() : MachineFunctionPass(ID) {}
};
} // end anonymous namespace

char GlulxFoldStores::ID = 0;
INITIALIZE_PASS(GlulxFoldStores, DEBUG_TYPE,
                "Fold const-addr stores into operands",
                false, false)

FunctionPass *llvm::createGlulxFoldStores() {
  return new GlulxFoldStores();
}

static bool isStoreFoldBarrier(const MachineInstr &MI) {
  return MI.mayLoadOrStore() || MI.isCall() ||
         (MI.hasUnmodeledSideEffects() && !MI.isPseudoProbe());
}

bool GlulxFoldStores::runOnMachineFunction(MachineFunction &MF) {
  if (skipFunction(MF.getFunction()))
    return false;

  LLVM_DEBUG(dbgs() << "********** Fold Stores **********\n");
  LLVM_DEBUG(dbgs() << "********** Function: " << MF.getName() << '\n');

  MRI = &MF.getRegInfo();
  auto *TII = MF.getSubtarget().getInstrInfo();

  bool Changed = false;

  for (MachineBasicBlock &MBB : MF) {
    SmallDenseMap<Register, MachineInstr*, 16> FoldAsStoreUseCandidates;

    // Traverse basic block in reverse order.
    for (auto MII = std::prev(MBB.end()), S = MBB.begin(); MII != S;) {
      MachineInstr *MI = &*MII--;
      unsigned MIOpc = MI->getOpcode();

      // Check whether we can fold a later store into MI.
      if (!FoldAsStoreUseCandidates.empty()) {

        // We visit each operand even after successfully folding a previous
        // one.  This allows us to fold multiple stores into a single
        // instruction.
        unsigned MINumOps = MI->getNumOperands();
        for (unsigned i = 0; i != MINumOps; ++i) {
          const MachineOperand &MOp = MI->getOperand(i);
          if (!MOp.isReg() || !MOp.isDef())
            continue;
          Register FoldAsStoreUseReg = MOp.getReg();
          auto Use = FoldAsStoreUseCandidates.find(FoldAsStoreUseReg);
          if (Use != FoldAsStoreUseCandidates.end()) {
            // We need to fold load after optimizeCmpInstr, since
            // optimizeCmpInstr can enable folding by converting SUB to CMP.
            // Save FoldAsLoadDefReg because optimizeLoadInstr() resets it and
            // we need it for markUsesInDebugValueAsUndef().
            Register FoldedReg = FoldAsStoreUseReg;
            MachineInstr *CopyMI = Use->second;
            assert(CopyMI->getOpcode() == Glulx::copy_rm);
            auto &StoreOp = CopyMI->getOperand(1);

            // Perform the fold.
            StoreOp.setTargetFlags(GlulxII::MO_DEREFERENCE);
            MachineInstr *FoldMI =
                MF.CreateMachineInstr(TII->get(MIOpc), MI->getDebugLoc(), true);
            MachineInstrBuilder MIB(MF, FoldMI);
            for (unsigned j = 0; j != MINumOps; ++j)
              MIB.add(j == i ? StoreOp : MI->getOperand(j));
            // Copy the memoperands from the store to the folded instruction.
            if (MI->memoperands_empty()) {
              FoldMI->setMemRefs(MF, CopyMI->memoperands());
            } else {
              // Handle the rare case of folding multiple stores.
              FoldMI->setMemRefs(MF, MI->memoperands());
              for (MachineInstr::mmo_iterator I = CopyMI->memoperands_begin(),
                                              E = CopyMI->memoperands_end();
                   I != E; ++I) {
                FoldMI->addMemOperand(MF, *I);
              }
            }
            MBB.insert(MI, FoldMI);

            LLVM_DEBUG(dbgs() << "Replacing: " << *MI);
            LLVM_DEBUG(dbgs() << "     With: " << *FoldMI);
            // Update the call site info.
            if (MI->shouldUpdateCallSiteInfo())
              MI->getMF()->moveCallSiteInfo(MI, FoldMI);
            MI->eraseFromParent();
            CopyMI->eraseFromParent();
            MRI->markUsesInDebugValueAsUndef(FoldedReg);
            FoldAsStoreUseCandidates.erase(FoldedReg);
            ++NumStoreFold;

            // MI is replaced with FoldMI so we can continue trying to fold
            Changed = true;
            MI = FoldMI;
          }
        }
      }

      // If we run into an instruction we can't fold across, discard
      // the store candidates.  Note: We might be able to fold *into* this
      // instruction, so this needs to be after the folding logic.
      if (isStoreFoldBarrier(*MI)) {
        LLVM_DEBUG(dbgs() << "Encountered store fold barrier on " << *MI);
        FoldAsStoreUseCandidates.clear();
      }

      // Check whether MI is a store candidate for folding into an earlier
      // instruction.
      if (MIOpc == Glulx::copy_rm) {
        const auto &MO = MI->getOperand(0);
        if (MO.isReg()) {
          Register Reg = MO.getReg();
          if (MRI->hasOneNonDBGUser(Reg))
            FoldAsStoreUseCandidates.insert(std::make_pair(Reg, MI));
        }
      }
    }
  }

  return Changed;
}
