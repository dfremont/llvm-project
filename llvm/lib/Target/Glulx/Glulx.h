//=== Glulx.h - Top-level interface for Glulx representation ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in
// the LLVM Glulx backend.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Glulx_Glulx_H
#define LLVM_LIB_TARGET_Glulx_Glulx_H

#include "MCTargetDesc/GlulxMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
  class FunctionPass;

  // ISel and immediate followup passes.
  FunctionPass *createGlulxArgumentMove();

  // Late passes.
  FunctionPass *createGlulxPrepareForLiveIntervals();
  FunctionPass *createGlulxOptimizeLiveIntervals();
  FunctionPass *createGlulxRegColoring();
  FunctionPass *createGlulxExplicitLocals();

  // PassRegistry initialization declarations.
  void initializeGlulxArgumentMovePass(PassRegistry &);
  void initializeGlulxPrepareForLiveIntervalsPass(PassRegistry &);
  void initializeGlulxOptimizeLiveIntervalsPass(PassRegistry &);
  void initializeGlulxRegColoringPass(PassRegistry &);
  void initializeGlulxExplicitLocalsPass(PassRegistry &);

} // end namespace llvm;

#endif // end LLVM_LIB_TARGET_Glulx_Glulx_H
