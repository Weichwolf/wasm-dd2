#!/usr/bin/env bash
# Prefer an installed compiler; fall back to the SDK used by the original setup.
if ! command -v emcc >/dev/null 2>&1; then
  DD2_EMSDK_ENV="${EMSDK:-$HOME/Git/emsdk}/emsdk_env.sh"
  if [ -f "$DD2_EMSDK_ENV" ]; then
    source "$DD2_EMSDK_ENV" >/dev/null 2>&1
  fi
  if ! command -v emcc >/dev/null 2>&1; then
    echo 'Emscripten fehlt: emcc installieren oder EMSDK auf das SDK-Verzeichnis setzen.' >&2
    return 1
  fi
fi
