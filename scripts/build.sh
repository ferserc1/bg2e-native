#!/bin/bash

cd engine/bg2e-js
npm install
gulp build

cd ../../engine/bg2e-js-physics
npm install
gulp build
