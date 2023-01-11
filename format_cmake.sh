#!/bin/bash

find base plugins applications -iname CMakeLists.txt -exec cmake-format {} -i \;

find cmake/modules -iname Dabc*.cmake -exec cmake-format {} -i \;
