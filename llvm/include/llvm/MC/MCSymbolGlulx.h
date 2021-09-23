//===-- llvm/MC/MCSymbolGlulx.h - Glulx Machine Code Symbols ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the MCSymbolGlulx class
///
//===----------------------------------------------------------------------===//
#ifndef LLVM_MC_MCSYMBOLGLULX_H
#define LLVM_MC_MCSYMBOLGLULX_H

#include "llvm/MC/MCSymbol.h"

namespace llvm {

class MCSymbolGlulx : public MCSymbol {
public:
  MCSymbolGlulx(const StringMapEntry<bool> *Name, bool IsTemporary)
      : MCSymbol(SymbolKindGlulx, Name, IsTemporary) {}
  static bool classof(const MCSymbol *S) { return S->isGlulx(); }
};
} // end namespace llvm

#endif
