//===-- GlulxTargetObjectFile.h - Glulx Object Info -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Glulx_GlulxTARGETOBJECTFILE_H
#define LLVM_LIB_TARGET_Glulx_GlulxTARGETOBJECTFILE_H

#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

namespace llvm {
class GlulxTargetObjectFile : public TargetLoweringObjectFile {
public:
  GlulxTargetObjectFile();
  ~GlulxTargetObjectFile() override = default;

  MCSection *SelectSectionForGlobal(const GlobalObject *GO, SectionKind Kind,
                                    const TargetMachine &TM) const override;
  MCSection *getExplicitSectionGlobal(const GlobalObject *GO, SectionKind Kind,
                                      const TargetMachine &TM) const override;

};
} // end namespace llvm

#endif // end LLVM_LIB_TARGET_Glulx_GlulxTARGETOBJECTFILE_H
