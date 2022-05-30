#!/usr/bin/env bash
# Generate gif from screenshots
# By: Melroy van den Berg
convert -layers OptimizePlus -delay 250 -loop 0 ./misc/winegui_*.png ./misc/winegui_screenshots.gif
