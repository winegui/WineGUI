#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Used for memory leak analysis,
# be-aware that you will get a lot of false positives messages due to GTK/Glib

cd ./build/bin
G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind \
--suppressions=/usr/share/glib-2.0/valgrind/glib.supp \
--leak-check=full --track-origins=yes \
./winegui
