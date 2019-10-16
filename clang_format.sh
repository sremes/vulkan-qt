#!/usr/bin/env bash

find . -iname "*.h" -or -iname "*.cpp" -print0 | xargs -0 clang-format --style=file -i