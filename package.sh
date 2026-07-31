#!/bin/bash

cp build/magritte .
zip magritte.zip README.md PROCESSORS.md USAGE.md magritte patterns/*
rm magritte
