//===-- llvm/MC/MCSectionGlulx.h - Glulx Machine Code Sections ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file declares the MCSectionGlulx class, which contains all of the
/// necessary machine code sections for the Glulx file format.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_MC_MCSECTIONGLULX_H
#define LLVM_MC_MCSECTIONGLULX_H

#include "llvm/MC/MCSection.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

class MCExpr;

class MCSectionGlulx final : public MCSection {
private:
  friend class MCContext;
  MCSectionGlulx(StringRef Name, SectionKind K)
      : MCSection(SV_Glulx, Name, K, nullptr) {}

public:
  void PrintSwitchToSection(const MCAsmInfo &MAI, const Triple &T,
                            raw_ostream &OS,
                            const MCExpr *Subsection) const override {
    llvm_unreachable("custom sections not allowed in Glulx");
  }

  bool UseCodeAlign() const override { return false; }

  bool isVirtualSection() const override { return false; }

  static bool classof(const MCSection *S) { return S->getVariant() == SV_Glulx; }
};
} // end namespace llvm

#endif
