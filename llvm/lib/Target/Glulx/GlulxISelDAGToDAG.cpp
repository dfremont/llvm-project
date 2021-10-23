//===-- GlulxISelDAGToDAG.cpp - A Dag to Dag Inst Selector for Glulx ------===//
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

#include "GlulxISelDAGToDAG.h"
#include "GlulxSubtarget.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/SelectionDAGISel.h"

using namespace llvm;

#define DEBUG_TYPE "Glulx-isel"

bool GlulxDAGToDAGISel::runOnMachineFunction(MachineFunction &MF) {
  Subtarget = &static_cast<const GlulxSubtarget &>(MF.getSubtarget());
  return SelectionDAGISel::runOnMachineFunction(MF);
}

void GlulxDAGToDAGISel::Select(SDNode *Node) {
  unsigned Opcode = Node->getOpcode();

  // If we have a custom node, we already have selected!
  if (Node->isMachineOpcode()) {
    LLVM_DEBUG(errs() << "== "; Node->dump(CurDAG); errs() << "\n");
    Node->setNodeId(-1);
    return;
  }

  // Instruction Selection not handled by the auto-generated tablegen selection
  // should be handled here.
  switch(Opcode) {
  default: break;
  }

  // Use auto-generated selection from tablegen.
  SelectCode(Node);
}

// Used to select any value that can be an instruction operand.
// (Which is in fact *any* value; but some require unwrapping.)
bool GlulxDAGToDAGISel::SelectAny(SDValue In, SDValue &Out) {
  SDLoc DL(In);
  unsigned Opcode = In.getOpcode();
  if (Opcode == GlulxISD::GA_WRAPPER) {
    Out = In.getOperand(0);
  } else if (auto *CN = dyn_cast<ConstantSDNode>(In)) {
    Out = CurDAG->getTargetConstant(CN->getSExtValue(), DL, MVT::i32);
  } else if (auto *FCN = dyn_cast<ConstantFPSDNode>(In)) {
    Out = CurDAG->getTargetConstantFP(FCN->getValueAPF(), DL, MVT::f32);
  } else {
    Out = In;
  }

  return true;
}
