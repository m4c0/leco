#pragma once
#include "clang/Lex/Pragma.h"

namespace dag {
class node;
}

struct ns_pragma : public clang::PragmaNamespace {
  ns_pragma();
  ns_pragma(dag::node *n);
};
