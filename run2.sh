#!/bin/bash
# Usage: ./run2.sh <basename>
# Example: ./run2.sh sample
# Requires: <basename>.city  <basename>.satoutput
# Produces: <basename>.metromap

BASENAME=$1
if [ -z "$BASENAME" ]; then
  echo "Usage: ./run2.sh <basename>"
  exit 1
fi

./run2 "$BASENAME.city" "$BASENAME.satoutput" "$BASENAME.metromap"

