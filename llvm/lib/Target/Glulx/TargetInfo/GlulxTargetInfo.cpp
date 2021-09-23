//===-- GlulxTargetInfo.cpp - Glulx Target Implementation -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/GlulxTargetInfo.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

Target &llvm::getTheGlulxTarget() {
  static Target TheGlulxTarget;
  return TheGlulxTarget;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeGlulxTargetInfo() {
  RegisterTarget<Triple::glulx> X(getTheGlulxTarget(), "glulx",
                                  "Glulx virtual machine", "Glulx");
}
