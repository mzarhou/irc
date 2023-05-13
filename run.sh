#!/usr/bin/env zsh

cat <(echo "pass test\nuser username$1 0 * * \nnick nickname$1") - | nc localhost 6667
