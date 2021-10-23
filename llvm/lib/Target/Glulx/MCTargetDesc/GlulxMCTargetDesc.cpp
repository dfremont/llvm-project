//===-- GlulxMCTargetDesc.cpp - Glulx Target Descriptions -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides Glulx specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "GlulxMCTargetDesc.h"
#include "GlulxInstPrinter.h"
#include "GlulxMCAsmInfo.h"
#include "GlulxTargetStreamer.h"
#include "TargetInfo/GlulxTargetInfo.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "GlulxGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "GlulxGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "GlulxGenRegisterInfo.inc"

static MCInstrInfo *createGlulxMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitGlulxMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createGlulxMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  return X;
}

static MCSubtargetInfo *
createGlulxMCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  return createGlulxMCSubtargetInfoImpl(TT, CPU, CPU, FS);
}

static MCInstPrinter *createGlulxMCInstPrinter(const Triple &T,
                                               unsigned SyntaxVariant,
                                               const MCAsmInfo &MAI,
                                               const MCInstrInfo &MII,
                                               const MCRegisterInfo &MRI) {
  return new GlulxInstPrinter(MAI, MII, MRI);
}

static MCAsmInfo *createGlulxMCAsmInfo(const MCRegisterInfo &MRI,
                                       const Triple &TT,
                                       const MCTargetOptions &Options) {
  return new GlulxMCAsmInfo(TT);
}

static MCTargetStreamer *createGlulxMCTargetStreamer(MCStreamer &S,
                                                     formatted_raw_ostream &OS,
                                                     MCInstPrinter *InstPrint,
                                                     bool IsVerboseAsm) {
  return new GlulxTargetAsmStreamer(S, OS);
}

extern "C" void LLVMInitializeGlulxTargetMC() {
  for (Target *T : {&getTheGlulxTarget()}) {
    // Register the MC asm info.
    TargetRegistry::RegisterMCAsmInfo(*T, createGlulxMCAsmInfo);

    // Register the MC instruction info.
    TargetRegistry::RegisterMCInstrInfo(*T, createGlulxMCInstrInfo);

    // Register the MC register info.
    TargetRegistry::RegisterMCRegInfo(*T, createGlulxMCRegisterInfo);

    // Register the MC subtarget info.
    TargetRegistry::RegisterMCSubtargetInfo(*T, createGlulxMCSubtargetInfo);

    // Register the MCInstPrinter.
    TargetRegistry::RegisterMCInstPrinter(*T, createGlulxMCInstPrinter);

    // Register the MCTargetStreamer.
    TargetRegistry::RegisterAsmTargetStreamer(*T, createGlulxMCTargetStreamer);
  }
}
