//===-- GlulxSubtarget.cpp - Glulx Subtarget Information --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Glulx specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "Glulx.h"
#include "GlulxSubtarget.h"
#include "GlulxMachineFunctionInfo.h"
#include "GlulxRegisterInfo.h"
#include "GlulxTargetMachine.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "Glulx-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "GlulxGenSubtargetInfo.inc"

GlulxSubtarget::GlulxSubtarget(const Triple &TT, StringRef CPU,
                               StringRef TuneCPU, StringRef FS,
                               const TargetMachine &TM)
    : GlulxGenSubtargetInfo(TT, CPU, TuneCPU, FS),
      TSInfo(),
      InstrInfo(initializeSubtargetDependencies(TT, CPU, TuneCPU, FS, TM)),
      FrameLowering(*this),
      TLInfo(TM, *this),
      RegInfo(*this) { }


GlulxSubtarget &
GlulxSubtarget::initializeSubtargetDependencies(const Triple &TT, StringRef CPU,
                                                StringRef TuneCPU, StringRef FS,
                                                const TargetMachine &TM) {
  // Parse features string.
  ParseSubtargetFeatures(CPU, TuneCPU, FS);

  return *this;
}
