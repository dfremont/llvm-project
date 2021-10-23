//===-- GlulxSelectionDAGInfo.h - Glulx SelectionDAG Info -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the Glulx subclass for SelectionDAGTargetInfo.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_GLULX_GLULXSELECTIONDAGINFO_H
#define LLVM_LIB_TARGET_GLULX_GLULXSELECTIONDAGINFO_H

#include "llvm/CodeGen/SelectionDAGTargetInfo.h"

namespace llvm {

class GlulxSelectionDAGInfo : public SelectionDAGTargetInfo {
public:
  SDValue EmitTargetCodeForMemcpy(SelectionDAG &DAG, const SDLoc &dl,
                                  SDValue Chain, SDValue Dst, SDValue Src,
                                  SDValue Size, Align Alignment,
                                  bool isVolatile, bool AlwaysInline,
                                  MachinePointerInfo DstPtrInfo,
                                  MachinePointerInfo SrcPtrInfo) const override;
  SDValue EmitTargetCodeForMemmove(SelectionDAG &DAG, const SDLoc &dl,
                                   SDValue Chain, SDValue Dst, SDValue Src,
                                   SDValue Size, Align Alignment,
                                   bool isVolatile,
                                   MachinePointerInfo DstPtrInfo,
                                   MachinePointerInfo SrcPtrInfo) const override;

  SDValue EmitTargetCodeForMemset(SelectionDAG &DAG, const SDLoc &dl,
                                  SDValue Chain, SDValue Dst, SDValue Src,
                                  SDValue Size, Align Alignment,
                                  bool isVolatile,
                                  MachinePointerInfo DstPtrInfo) const override;
};

}

#endif
