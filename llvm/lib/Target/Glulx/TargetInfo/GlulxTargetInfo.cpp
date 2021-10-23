//===-- GlulxTargetInfo.cpp - Glulx Target Implementation -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/GlulxTargetInfo.h"
#include "MCAsmStreamer/GlulxMCAsmStreamer.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

class GlulxTarget : public Target {
  MCStreamer *createAsmStreamer(MCContext &Ctx,
                                std::unique_ptr<formatted_raw_ostream> OS,
                                bool IsVerboseAsm, bool UseDwarfDirectory,
                                MCInstPrinter *InstPrint,
                                std::unique_ptr<MCCodeEmitter> &&CE,
                                std::unique_ptr<MCAsmBackend> &&TAB,
                                bool ShowInst) const override;
};

MCStreamer *GlulxTarget::createAsmStreamer(MCContext &Ctx,
                              std::unique_ptr<formatted_raw_ostream> OS,
                              bool IsVerboseAsm, bool UseDwarfDirectory,
                              MCInstPrinter *InstPrint,
                              std::unique_ptr<MCCodeEmitter> &&CE,
                              std::unique_ptr<MCAsmBackend> &&TAB,
                              bool ShowInst) const {
  formatted_raw_ostream &OSRef = *OS;
  MCStreamer *S = new GlulxMCAsmStreamer(Ctx, std::move(OS), IsVerboseAsm,
                                         UseDwarfDirectory, InstPrint,
                                         std::move(CE), std::move(TAB),
                                         ShowInst);
  createAsmTargetStreamer(*S, OSRef, InstPrint, IsVerboseAsm);
  return S;
}

Target &llvm::getTheGlulxTarget() {
  static GlulxTarget TheGlulxTarget;
  return TheGlulxTarget;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeGlulxTargetInfo() {
  RegisterTarget<Triple::glulx> X(getTheGlulxTarget(), "glulx",
                                  "Glulx virtual machine", "Glulx");
}
