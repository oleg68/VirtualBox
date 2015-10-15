#!/bin/sh
## @file
# For development, builds and loads all the host drivers.
#

#
# Copyright (C) 2010-2015 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

MY_DIR=`dirname "$0"`
MY_DIR=`cd "${MY_DIR}" && pwd`
if [ ! -d "${MY_DIR}" ]; then
    echo "Cannot find ${MY_DIR} or it's not a directory..."
    exit 1;
fi

if test ${#} -eq 0; then
    sudo "${MY_DIR}/vboxdrv.sh" setup
elif test $* = 1 && test "${1}" = -u; then
    sudo "${MY_DIR}/vboxdrv.sh" cleanup
else
    echo "Usage: loadall.sh [-u]"
    exit 1
fi
