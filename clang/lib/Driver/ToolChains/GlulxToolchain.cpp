//===--- GlulxToolchain.cpp - Glulx ToolChain Implementations ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "GlulxToolchain.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "llvm/Option/ArgList.h"

using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace clang;
using namespace llvm::opt;

GlulxToolChain::GlulxToolChain(const Driver &D, const llvm::Triple &Triple,
                               const ArgList &Args)
    : ToolChain(D, Triple, Args) {
  // ProgramPaths are found via 'PATH' environment variable.
}

bool GlulxToolChain::isPICDefault() const { return false; }

bool GlulxToolChain::isPIEDefault() const { return false; }

bool GlulxToolChain::isPICDefaultForced() const { return true; }

bool GlulxToolChain::SupportsProfiling() const { return false; }

bool GlulxToolChain::hasBlocksRuntime() const { return false; }
