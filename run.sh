#!/usr/bin/env zsh

cat <(echo "pass test\nuser us$1 0 * * \nnick nk$1") - | nc localhost 6667
