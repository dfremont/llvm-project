//===---- GlulxISelDAGToDAG.h - A Dag to Dag Inst Selector for Glulx ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines an instruction selector for the Glulx target.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Glulx_GlulxISELDAGTODAG_H
#define LLVM_LIB_TARGET_Glulx_GlulxISELDAGTODAG_H

#include "GlulxSubtarget.h"
#include "GlulxTargetMachine.h"
#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {
class GlulxDAGToDAGISel : public SelectionDAGISel {
public:
  explicit GlulxDAGToDAGISel(GlulxTargetMachine &TM, CodeGenOpt::Level OL)
      : SelectionDAGISel(TM, OL), Subtarget(nullptr) {}

  // Pass Name
  StringRef getPassName() const override {
    return "Glulx DAG->DAG Pattern Instruction Selection";
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  void Select(SDNode *Node) override;

  // ComplexPatterns
  bool SelectAny(SDValue In, SDValue &Out);

#include "GlulxGenDAGISel.inc"

private:
  const GlulxSubtarget *Subtarget;
};
}

#endif // end LLVM_LIB_TARGET_Glulx_GlulxISELDAGTODAG_H
