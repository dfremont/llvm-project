//===--- Glulx.h - Declare Glulx target feature support ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares GlulxTargetInfo objects.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_GLULX_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_GLULX_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/Compiler.h"

namespace clang {
namespace targets {

class LLVM_LIBRARY_VISIBILITY GlulxTargetInfo : public TargetInfo {
  static const Builtin::Info BuiltinInfo[];

public:
  GlulxTargetInfo(const llvm::Triple &Triple, const TargetOptions &)
    : TargetInfo(Triple) {
    // Description string has to be kept in sync with backend string at
    // llvm/lib/Target/Glulx/GlulxTargetMachine.cpp
    resetDataLayout("E"
                    // ELF name mangling
                    "-m:e"
                    // 32-bit pointers, 8-bit aligned
                    "-p:32:8"
                    // 32-bit integers, 8-bit aligned
                    "-i32:8"
                    // 32-bit native integer width i.e register are 32-bit
                    "-n32"
                    // 32-bit floats, 8-bit aligned
                    "-f32:8"
                    // 8-bit natural stack alignment
                    "-S8"
    );
    TLSSupported = false;
    VLASupported = false;
    PointerAlign = 8; IntAlign = 8; LongAlign = 8; FloatAlign = 8;
    LongLongWidth = 32; LongLongAlign = 8;
    SuitableAlign = 8;
    DefaultAlignForAttributeAligned = 8;
    DoubleWidth = 32; DoubleAlign = 8;
    DoubleFormat = &llvm::APFloat::IEEEsingle();
    LongDoubleWidth = 32; LongDoubleAlign = 8;
    LongDoubleFormat = &llvm::APFloat::IEEEsingle();
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;

  ArrayRef<const char *> getGCCRegNames() const final { return None; }

  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const final {
    return None;
  }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::VoidPtrBuiltinVaList;
  }

  ArrayRef<Builtin::Info> getTargetBuiltins() const override;

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &info) const override {
    return false;
  }

  const char *getClobbers() const override {
    return "";
  }
};

} // namespace targets
} // namespace clang

#endif // LLVM_CLANG_LIB_BASIC_TARGETS_GLULX_H
