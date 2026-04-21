find src -name "*.c" ! -path '*/lib/cbmp.c' | \
          xargs clang-tidy -checks='-*,readability-*,performance-*' --warnings-as-errors
