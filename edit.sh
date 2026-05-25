#!/bin/sh
vi $(find src \( -name "*.c" -o -name "*.cpp" \) -not -name "_*")
