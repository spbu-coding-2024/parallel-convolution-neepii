find src -name "*.c" | \
          xargs clang-tidy -checks='-*,readability-*,performance-*' --warnings-as-errors
