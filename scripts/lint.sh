find src -name "*.c" ! -path '*/lib/cbmp.c' | \
          xargs clang-tidy -p "build/" -checks='-*,readability-*,performance-*' --warnings-as-errors
