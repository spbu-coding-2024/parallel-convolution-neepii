find ./src -name '*.c' -not -path './src/external/*' | \
          xargs -I {} clang-tidy -p 'build/' {} -- -Isrc/external -Isrc/lib
