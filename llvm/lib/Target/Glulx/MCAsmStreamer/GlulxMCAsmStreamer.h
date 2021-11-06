//===-- GlulxMCAsmStreamer.h - Glulx Asm Streamer ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides a Glulx-specific MCAsmStreamer.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_GLULX_MCTARGETDESC_GLULXMCASMSTREAMER_H
#define LLVM_LIB_TARGET_GLULX_MCTARGETDESC_GLULXMCASMSTREAMER_H

#include "llvm/MC/MCAsmStreamer.h"

using namespace llvm;

class GlulxMCAsmStreamer : public MCAsmStreamer {
private:
  bool EOLPending = false;

public:
  GlulxMCAsmStreamer(MCContext &Context, std::unique_ptr<formatted_raw_ostream> os,
                bool isVerboseAsm, bool useDwarfDirectory,
                MCInstPrinter *printer, std::unique_ptr<MCCodeEmitter> emitter,
                std::unique_ptr<MCAsmBackend> asmbackend, bool showInst)
      : MCAsmStreamer(Context, std::move(os), isVerboseAsm, useDwarfDirectory,
                      printer, std::move(emitter), std::move(asmbackend),
                      showInst) {};

  void emitLabel(MCSymbol *Symbol, SMLoc Loc = SMLoc()) override;
};

#endif // LLVM_LIB_TARGET_GLULX_MCTARGETDESC_GLULXMCASMSTREAMER_H
