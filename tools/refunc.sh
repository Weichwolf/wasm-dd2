#!/usr/bin/env bash
# print decompiled function(s) whose header matches the given name
awk -v pat="$1" '
  /^\/\* ===== / { p = ($0 ~ pat) }
  p { print }
' re_out/dd2_decomp.c
