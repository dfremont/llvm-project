//===--- GlulxUnsignedToSignedDivision.cpp - Convert udiv to sdiv, etc. ---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Convert unsigned division/remainder to signed versions when possible.
//
/// Glulx does not provide unsigned division/remainder instructions, so in
/// general a libcall is required to implement them. Here we attempt to replace
/// the unsigned operations with signed ones when they are known to be
/// equivalent.
///
//===----------------------------------------------------------------------===//

#include "Glulx.h"
#include "GlulxMachineFunctionInfo.h"
#include "GlulxSubtarget.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/ProfileSummaryInfo.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/CodeGen/GCMetadata.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/StackProtector.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "unsigned-to-signed-division"

namespace {
class UnsignedToSignedDivision final : public FunctionPass {
  StringRef getPassName() const override {
    return "Convert unsigned to signed division";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addPreserved<AAResultsWrapperPass>();
    AU.addPreserved<GCModuleInfo>();
    AU.addPreserved<StackProtector>();
    AU.addPreserved<TargetLibraryInfoWrapperPass>();
    AU.addPreserved<TargetTransformInfoWrapperPass>();
    AU.addPreserved<BranchProbabilityInfoWrapperPass>();
    AU.addPreserved<ProfileSummaryInfoWrapperPass>();
    FunctionPass::getAnalysisUsage(AU);
  }

  bool runOnFunction(Function &MF) override;

public:
  static char ID; // Pass identification, replacement for typeid
  UnsignedToSignedDivision() : FunctionPass(ID) {}
};
} // end anonymous namespace

char UnsignedToSignedDivision::ID = 0;
INITIALIZE_PASS(UnsignedToSignedDivision, DEBUG_TYPE,
                "Convert unsigned to signed division", false, false)

FunctionPass *llvm::createUnsignedToSignedDivisionPass() {
  return new UnsignedToSignedDivision();
}

bool UnsignedToSignedDivision::runOnFunction(Function &F) {
  LLVM_DEBUG(dbgs() << "********** Convert unsigned to signed division **********\n"
                       "********** Function: "
                    << F.getName() << '\n');

  bool Changed = false;
  auto Layout = F.getEntryBlock().getModule()->getDataLayout();

  for (auto &B : F) {
    BasicBlock::iterator Next;
    for (auto II = B.begin(), BE = B.end(); II != BE; II = Next) {
      Next = std::next(II);   // precompute in case we delete this instruction
      Instruction *I = &*II;
      BinaryOperator *Op = dyn_cast<BinaryOperator>(I);
      if (!Op)
        continue;
      unsigned Opcode = Op->getOpcode();
      if (Opcode != Instruction::UDiv && Opcode != Instruction::URem)
        continue;

      // If the sign bits of the operands are both zero, then their unsigned and
      // signed representations are equal, so we can replace the unsigned
      // operation with its signed counterpart.
      Value *Op0 = I->getOperand(0), *Op1 = I->getOperand(1);
      APInt Mask(APInt::getSignMask(I->getType()->getScalarSizeInBits()));
      if (MaskedValueIsZero(Op1, Mask, Layout) &&
          MaskedValueIsZero(Op0, Mask, Layout)) {
        IRBuilder<> Builder(I);
        Builder.SetCurrentDebugLocation(I->getDebugLoc());
        Value *NewInst;
        if (Opcode == Instruction::UDiv)
          NewInst = Builder.CreateSDiv(Op0, Op1);
        else
          NewInst = Builder.CreateSRem(Op0, Op1);
        I->replaceAllUsesWith(NewInst);
        I->eraseFromParent();
        Changed = true;
      }
    }
  }

  return Changed;
}
