//===-- GlulxExplicitLocals.cpp - Make Locals Explicit --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Converts virtual registers to Glulx locals and emits function headers.
///
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/GlulxMCTargetDesc.h"
#include "Glulx.h"
#include "GlulxMachineFunctionInfo.h"
#include "GlulxSubtarget.h"
#include "llvm/CodeGen/MachineBlockFrequencyInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define DEBUG_TYPE "glulx-explicit-locals"

namespace {
class GlulxExplicitLocals final : public MachineFunctionPass {
  StringRef getPassName() const override {
    return "Glulx Explicit Locals";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addPreserved<MachineBlockFrequencyInfo>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

public:
  static char ID; // Pass identification, replacement for typeid
  GlulxExplicitLocals() : MachineFunctionPass(ID) {}
};
} // end anonymous namespace

char GlulxExplicitLocals::ID = 0;
INITIALIZE_PASS(GlulxExplicitLocals, DEBUG_TYPE,
                "Convert registers to Glulx locals", false, false)

FunctionPass *llvm::createGlulxExplicitLocals() {
  return new GlulxExplicitLocals();
}

static void checkFrameBase(GlulxFunctionInfo &MFI, unsigned Local,
                           unsigned Reg) {
  // Mark a local for the frame base vreg.
  if (MFI.isFrameBaseVirtual() && Reg == MFI.getFrameBaseVreg()) {
    LLVM_DEBUG({
      dbgs() << "Allocating local " << Local << "for VReg "
             << Register::virtReg2Index(Reg) << '\n';
    });
    MFI.setFrameBaseLocal(Local);
  }
}

/// Return a local id number for the given register, assigning it a new one
/// if it doesn't yet have one.
static unsigned getLocalId(DenseMap<unsigned, unsigned> &Reg2Local,
                           GlulxFunctionInfo &MFI, unsigned &CurLocal,
                           unsigned Reg) {
  auto P = Reg2Local.insert(std::make_pair(Reg, CurLocal));
  if (P.second) {
    checkFrameBase(MFI, CurLocal, Reg);
    ++CurLocal;
  }
  return P.first->second;
}

bool GlulxExplicitLocals::runOnMachineFunction(MachineFunction &MF) {
  LLVM_DEBUG(dbgs() << "********** Make Locals Explicit **********\n"
                       "********** Function: "
                    << MF.getName() << '\n');

  MachineRegisterInfo &MRI = MF.getRegInfo();
  GlulxFunctionInfo &MFI = *MF.getInfo<GlulxFunctionInfo>();

  // Map non-stackified virtual registers to their local ids.
  DenseMap<unsigned, unsigned> Reg2Local;

  // Handle ARGUMENTS first to ensure that they get the designated numbers.
  for (MachineBasicBlock::iterator I = MF.begin()->begin(),
                                   E = MF.begin()->end();
       I != E;) {
    MachineInstr &MI = *I++;
    if (!Glulx::isArgument(MI.getOpcode()))
      break;
    Register Reg = MI.getOperand(0).getReg();
    assert(!MFI.isVRegStackified(Reg));
    auto Local = static_cast<unsigned>(MI.getOperand(1).getImm());
    Reg2Local[Reg] = Local;
    checkFrameBase(MFI, Local, Reg);

//    // Update debug value to point to the local before removing.
//    WebAssemblyDebugValueManager(&MI).replaceWithLocal(Local);

    MI.eraseFromParent();
  }

  // Start assigning local numbers after the last parameter and after any
  // already-assigned locals.
  unsigned CurLocal = static_cast<unsigned>(MFI.getParams().size());
  CurLocal += static_cast<unsigned>(MFI.getLocals().size());

  // Precompute the set of registers that are unused, so that we can insert
  // drops to their defs.
  BitVector UseEmpty(MRI.getNumVirtRegs());
  for (unsigned I = 0, E = MRI.getNumVirtRegs(); I < E; ++I)
    UseEmpty[I] = MRI.use_empty(Register::index2VirtReg(I));

  // Visit each instruction in the function.
  for (MachineBasicBlock &MBB : MF) {
    for (MachineBasicBlock::iterator I = MBB.begin(), E = MBB.end(); I != E;) {
      MachineInstr &MI = *I++;
      assert(!Glulx::isArgument(MI.getOpcode()));

      if (MI.isDebugInstr() || MI.isLabel())
        continue;

      if (MI.getOpcode() == Glulx::IMPLICIT_DEF) {
        MI.eraseFromParent();
        continue;
      }

      for (auto &Def : MI.defs()) {
        Register OldReg = Def.getReg();
        if (UseEmpty[Register::virtReg2Index(OldReg)]) {
          Def.setReg(0);
        } else {
          unsigned LocalId = getLocalId(Reg2Local, MFI, CurLocal, OldReg);
          Def.setReg(INT32_MIN | LocalId);
        }
      }

      for (MachineOperand &MO : MI.explicit_uses()) {
        if (!MO.isReg())
          continue;
        Register OldReg = MO.getReg();
        unsigned LocalId = getLocalId(Reg2Local, MFI, CurLocal, OldReg);
        MO.setReg(INT32_MIN | LocalId);
      }
    }
  }

  // Emit function header.
  auto &ST = MF.getSubtarget<GlulxSubtarget>();
  const auto *TII = ST.getInstrInfo();
  auto MBB = MF.begin();
  DebugLoc DL;
  BuildMI(*MBB, MBB->begin(), DL, TII->get(Glulx::MAKE_LFUNC))
      .addImm(CurLocal);  // total number of locals used

  return true;
}
