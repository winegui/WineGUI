#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Helper script for creating and uploading new Docker image to Docker Hub
# Create latest image
docker build -t danger89/gtk3-docker-cmake-ninja ./misc
# Push
docker push danger89/gtk3-docker-cmake-ninja
