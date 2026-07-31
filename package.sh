#!/bin/bash

cp build/magritte .
zip magritte.zip README.md STEPS.md USAGE.md magritte patterns/*
rm magritte
