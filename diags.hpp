#pragma once
#include "cl.hpp"
#include "clang/Basic/Diagnostic.h"

template <unsigned N>
static void diag_error(clang::DiagnosticsEngine &d, clang::SourceLocation loc,
                       const char (&msg)[N]) {
  auto d_id = d.getCustomDiagID(DiagnosticsEngine::Error, msg);
  d.Report(loc, d_id);
}
template <unsigned N>
static void diag_remark(clang::DiagnosticsEngine &d, clang::SourceLocation loc,
                        const char (&msg)[N]) {
  if (!is_verbose())
    return;

  auto d_id = d.getCustomDiagID(DiagnosticsEngine::Remark, msg);
  d.Report(loc, d_id);
}
