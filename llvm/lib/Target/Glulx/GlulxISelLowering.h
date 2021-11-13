//=== GlulxISelLowering.h - Glulx DAG Lowering Interface --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Glulx uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Glulx_GlulxISELLOWERING_H
#define LLVM_LIB_TARGET_Glulx_GlulxISELLOWERING_H

#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/IR/Function.h"

namespace llvm {
namespace GlulxISD {
enum NodeType {
  // Start the numbering from where ISD NodeType finishes.
  FIRST_NUMBER = ISD::BUILTIN_OP_END,

  CALL,
  TAILCALL,
  CALLF,
  CALLFI,
  CALLFII,
  CALLFIII,
  Ret,
  PUSH,
  GA_WRAPPER,
  SELECT_CC,
  MEMCPY,
  MEMCLR,
  JISNAN,
  JORDERED,
  BR_CC_FP,

  ARGUMENT,
};
}

class GlulxSubtarget;

class GlulxTargetLowering : public TargetLowering  {
public:
  explicit GlulxTargetLowering(const TargetMachine &TM,
                              const GlulxSubtarget &STI);

  const char *getTargetNodeName(unsigned Opcode) const override;

  bool isFPImmLegal(const APFloat &, EVT,
                    bool ForCodeSize = false) const override { return true; }
  bool isFsqrtCheap(SDValue X, SelectionDAG &DAG) const override {
    return true;
  }
  bool convertSelectOfConstantsToMath(EVT VT) const override { return true; }
  bool isSelectSupported(SelectSupportKind kind) const override {
    return false;   // eliminate select when possible since we don't have it
  }
  bool isLegalStoreImmediate(int64_t Value) const override {
    return isInt<32>(Value);
  }
  bool isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const override {
    return false;
  }
  bool isLegalAddressingMode(const DataLayout &DL, const AddrMode &AM,
                             Type *Ty, unsigned AddrSpace,
                             Instruction *I = nullptr) const override;

  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;
  void ReplaceNodeResults(SDNode *N,
                          SmallVectorImpl<SDValue> &Results,
                          SelectionDAG &DAG) const override;
  MachineBasicBlock *
  EmitInstrWithCustomInserter(MachineInstr &MI,
                              MachineBasicBlock *BB) const override;

protected:
  // Subtarget Info
  const GlulxSubtarget &Subtarget;

private:
  SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerExternalSymbol(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerFrameIndex(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerJumpTable(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerCopyToReg(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerConstantPool(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerRETURNADDR(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVASTART(SDValue Op, SelectionDAG &DAG) const;

  using RegsToPassVector = SmallVector<std::pair<unsigned, SDValue>, 8>;

  SDValue getGlobalAddressWrapper(SDValue GA,
                                  const GlobalValue *GV,
                                  SelectionDAG &DAG) const;

  SDValue LowerFormalArguments(SDValue Chain,
                         CallingConv::ID CallConv, bool IsVarArg,
                         const SmallVectorImpl<ISD::InputArg> &Ins,
                         const SDLoc &dl, SelectionDAG &DAG,
                         SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerReturn(SDValue Chain,
                      CallingConv::ID CallConv, bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals,
                      const SDLoc &dl, SelectionDAG &DAG) const override;
  bool CanLowerReturn(CallingConv::ID CallConv,
                      MachineFunction &MF, bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      LLVMContext &Context) const override;

  SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;
  bool isEligibleForTailCallOptimization(
      CCState &CCInfo, CallLoweringInfo &CLI, MachineFunction &MF,
      const SmallVector<CCValAssign, 16> &ArgLocs) const;
  SDValue LowerCallResult(SDValue Chain, SDValue InFlag,
                          CallingConv::ID CallConv, bool IsVarArg,
                          const SmallVectorImpl<ISD::InputArg> &Ins,
                          const SDLoc &dl, SelectionDAG &DAG,
                          SmallVectorImpl<SDValue> &InVals,
                          bool isThisReturn, SDValue ThisVal) const;
  SDValue LowerMemOpCallTo(SDValue Chain,
                           SDValue Arg, const SDLoc &dl,
                           SelectionDAG &DAG, const CCValAssign &VA,
                           ISD::ArgFlagsTy Flags) const;
};
}

#endif // end LLVM_LIB_TARGET_Glulx_GlulxISELLOWERING_H
