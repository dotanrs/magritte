#!/bin/bash

cp build/magritte .
zip magritte.zip README.md PROCESSORS.md USAGE.md magritte formulas/*
rm magritte
