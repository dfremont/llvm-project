//=- GlulxMachineFunctionInfo.cpp - WebAssembly Machine Function Info -=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file implements Glulx-specific per-machine-function
/// information.
///
//===----------------------------------------------------------------------===//

#include "GlulxMachineFunctionInfo.h"
#include "GlulxISelLowering.h"
#include "GlulxSubtarget.h"
#include "llvm/CodeGen/Analysis.h"
#include "llvm/CodeGen/WasmEHFuncInfo.h"
#include "llvm/Target/TargetMachine.h"
using namespace llvm;

GlulxFunctionInfo::~GlulxFunctionInfo() = default; // anchor.

void GlulxFunctionInfo::initWARegs(MachineRegisterInfo &MRI) {
  assert(WARegs.empty());
  unsigned Reg = UnusedReg;
  WARegs.resize(MRI.getNumVirtRegs(), Reg);
}

void llvm::Glulx::computeLegalValueVTs(const Function &F, const TargetMachine &TM,
                                Type *Ty, SmallVectorImpl<MVT> &ValueVTs) {
  const DataLayout &DL(F.getParent()->getDataLayout());
  const GlulxTargetLowering &TLI =
      *TM.getSubtarget<GlulxSubtarget>(F).getTargetLowering();
  SmallVector<EVT, 4> VTs;
  ComputeValueVTs(TLI, DL, Ty, VTs);

  for (EVT VT : VTs) {
    unsigned NumRegs = TLI.getNumRegisters(F.getContext(), VT);
    MVT RegisterVT = TLI.getRegisterType(F.getContext(), VT);
    for (unsigned I = 0; I != NumRegs; ++I)
      ValueVTs.push_back(RegisterVT);
  }
}

void llvm::Glulx::computeSignatureVTs(const FunctionType *Ty,
                               const Function *TargetFunc,
                               const Function &ContextFunc,
                               const TargetMachine &TM,
                               SmallVectorImpl<MVT> &Params,
                               SmallVectorImpl<MVT> &Results) {
  computeLegalValueVTs(ContextFunc, TM, Ty->getReturnType(), Results);

  MVT PtrVT = MVT::getIntegerVT(TM.createDataLayout().getPointerSizeInBits());
  if (Results.size() > 1) {
    // Glulx can't lower returns of multiple values. So replace multiple return
    // values with a pointer parameter.
    Results.clear();
    Params.push_back(PtrVT);
  }

  for (auto *Param : Ty->params())
    computeLegalValueVTs(ContextFunc, TM, Param, Params);
  if (Ty->isVarArg())
    Params.push_back(PtrVT);
}

//std::unique_ptr<wasm::WasmSignature>
//llvm::Glulx::signatureFromMVTs(const SmallVectorImpl<MVT> &Results,
//                        const SmallVectorImpl<MVT> &Params) {
//  auto Sig = std::make_unique<wasm::WasmSignature>();
//  valTypesFromMVTs(Results, Sig->Returns);
//  valTypesFromMVTs(Params, Sig->Params);
//  return Sig;
//}

yaml::GlulxFunctionInfo::GlulxFunctionInfo(
    const llvm::GlulxFunctionInfo &MFI)
    : CFGStackified(MFI.isCFGStackified()) {
  for (auto VT : MFI.getParams())
    Params.push_back(EVT(VT).getEVTString());
  for (auto VT : MFI.getResults())
    Results.push_back(EVT(VT).getEVTString());
}

void yaml::GlulxFunctionInfo::mappingImpl(yaml::IO &YamlIO) {
  MappingTraits<GlulxFunctionInfo>::mapping(YamlIO, *this);
}

