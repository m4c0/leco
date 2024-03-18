#include "pragma.hpp"
#include "clang/Lex/Preprocessor.h"

static clang::PragmaHandlerRegistry::Add<ns_pragma> x{
    "leco", "handles declarative include directory as well as ignoring other "
            "leco pragmas in normal builds"};
