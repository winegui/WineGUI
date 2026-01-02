#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Generate memory usage plots in massif format (https://valgrind.org/docs/manual/ms-manual.html),
# be-aware that you will get a lot of false positives messages due to GTK/Glib

cd ./build/bin
G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind \
--suppressions=/usr/share/glib-2.0/valgrind/glib.supp \
--suppressions=/usr/share/gtk-3.0/valgrind/gtk.supp \
--tool=massif \
./winegui
