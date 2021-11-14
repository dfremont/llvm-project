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

// Awful hack to save RAM/BSS sections for the end, since glasm doesn't allow
// switching back and forth between sections.
class GlulxStreamMultiplexer : public formatted_raw_ostream {
  SmallString<128> RAMSection, BSSSection;
  raw_svector_ostream RAMStream, BSSStream;
  std::unique_ptr<formatted_raw_ostream> MainOS;
  formatted_raw_ostream RAMOS, BSSOS;
  formatted_raw_ostream *CurrentOS;

public:
  GlulxStreamMultiplexer(std::unique_ptr<formatted_raw_ostream> OS)
      : formatted_raw_ostream(*OS->TheStream),
        RAMStream(RAMSection), BSSStream(BSSSection),
        MainOS(std::move(OS)), RAMOS(RAMStream), BSSOS(BSSStream),
        CurrentOS(&*MainOS) {};

  void write_impl(const char *Ptr, size_t Size) override {
    CurrentOS->write_impl(Ptr, Size);
  }

  void switchToROM() {
    CurrentOS = &*MainOS;
  }

  void switchToRAM() {
    CurrentOS = &RAMOS;
  }

  void switchToBSS() {
    CurrentOS = &BSSOS;
  }

  bool inBSS() const {
    return CurrentOS == &BSSOS;
  }

  void finish() {
    *MainOS << "\n!ram\n" << RAMSection;
    *MainOS << "\n!bss\n" << BSSSection;
  }

  ~GlulxStreamMultiplexer() override {
    flush();
  }
};

class GlulxMCAsmStreamer : public MCAsmStreamer {
private:
  bool EOLPending = false;

public:
  GlulxMCAsmStreamer(MCContext &Context, std::unique_ptr<formatted_raw_ostream> os,
                bool isVerboseAsm, bool useDwarfDirectory,
                MCInstPrinter *printer, std::unique_ptr<MCCodeEmitter> emitter,
                std::unique_ptr<MCAsmBackend> asmbackend, bool showInst)
      : MCAsmStreamer(Context,
                      std::make_unique<GlulxStreamMultiplexer>(std::move(os)),
                      isVerboseAsm, useDwarfDirectory,
                      printer, std::move(emitter),
                      std::move(asmbackend),
                      showInst) {};

  void changeSection(MCSection *Section, const MCExpr *Subsection) override;
  void emitLabel(MCSymbol *Symbol, SMLoc Loc = SMLoc()) override;
  bool emitSymbolAttribute(MCSymbol *Symbol, MCSymbolAttr Attribute) override;
  void emitCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                        unsigned ByteAlignment) override;
  void emitValueToAlignment(unsigned ByteAlignment, int64_t Value = 0,
                            unsigned ValueSize = 1,
                            unsigned MaxBytesToEmit = 0) override;
  void finishImpl() override;
};

#endif // LLVM_LIB_TARGET_GLULX_MCTARGETDESC_GLULXMCASMSTREAMER_H
