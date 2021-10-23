//===-- GlulxSelectionDAGInfo.cpp - Glulx SelectionDAG Info ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the GlulxSelectionDAGInfo class.
//
//===----------------------------------------------------------------------===//

#include "GlulxTargetMachine.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/IR/DerivedTypes.h"

using namespace llvm;

#define DEBUG_TYPE "glulx-selectiondag-info"

SDValue GlulxSelectionDAGInfo::EmitTargetCodeForMemcpy(
    SelectionDAG &DAG, const SDLoc &DL, SDValue Chain, SDValue Dst, SDValue Src,
    SDValue Size, Align Alignment, bool IsVolatile, bool AlwaysInline,
    MachinePointerInfo DstPtrInfo, MachinePointerInfo SrcPtrInfo) const {
  if (Size.getValueType() != MVT::i32) {
    // clang sometimes produces i64 sizes even though size_t is 32-bit???
    ConstantSDNode *V = dyn_cast<ConstantSDNode>(Size);
    if (!V || !isUInt<32>(V->getZExtValue()))
      report_fatal_error("unsupported 64-bit memcpy/memmove");
    Size = DAG.getConstant(V->getZExtValue(), DL, MVT::i32);
  }
  SDValue Copy = DAG.getNode(GlulxISD::MEMCPY, DL, MVT::Other,
                             Chain, Size, Src, Dst);
  return Copy.getValue(0);
}

SDValue GlulxSelectionDAGInfo::EmitTargetCodeForMemmove(
    SelectionDAG &DAG, const SDLoc &DL, SDValue Chain, SDValue Dst, SDValue Src,
    SDValue Size, Align Alignment, bool IsVolatile,
    MachinePointerInfo DstPtrInfo, MachinePointerInfo SrcPtrInfo) const {
  return EmitTargetCodeForMemcpy(DAG, DL, Chain, Dst, Src, Size, Alignment,
                                 IsVolatile, false, DstPtrInfo, SrcPtrInfo);
}

SDValue GlulxSelectionDAGInfo::EmitTargetCodeForMemset(
    SelectionDAG &DAG, const SDLoc &DL, SDValue Chain, SDValue Dst, SDValue Src,
    SDValue Size, Align Alignment, bool IsVolatile,
    MachinePointerInfo DstPtrInfo) const {
  ConstantSDNode *V = dyn_cast<ConstantSDNode>(Src);
  if (V && V->isNullValue()) {
    // assignment to constant zero; use mzero
    if (Size.getValueType() != MVT::i32)
      Size = DAG.getZExtOrTrunc(Size, DL, MVT::i32);
    SDValue Copy = DAG.getNode(GlulxISD::MEMCLR, DL, MVT::Other,
                               Chain, Size, Dst);
    return Copy.getValue(0);
  }
  return SDValue();   // can't handle this; use default strategy
}