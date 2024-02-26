#!/usr/bin/env bash
# Generate gif from screenshots
# By: Melroy van den Berg
convert -layers OptimizePlus -delay 250 -loop 0 ./misc/screenshots/*.png ./misc/winegui_screenshots.gif
