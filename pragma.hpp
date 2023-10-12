#pragma once
#include "clang/Lex/Pragma.h"

struct ns_pragma : public clang::PragmaNamespace {
  ns_pragma();
};
