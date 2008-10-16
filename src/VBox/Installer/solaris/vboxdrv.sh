#!/bin/sh
# Sun xVM VirtualBox
# VirtualBox kernel module control script for Solaris.
#
# Copyright (C) 2007-2008 Sun Microsystems, Inc.
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
# Clara, CA 95054 USA or visit http://www.sun.com if you need
# additional information or have any questions.
#

STOPPING=""
SILENTUNLOAD=""
MODNAME="vboxdrv"
FLTMODNAME="vboxflt"
MODDIR32="/platform/i86pc/kernel/drv"
MODDIR64=$MODDIR32/amd64

abort()
{
    echo 1>&2 "## $1"
    exit 1
}

info()
{
    echo 1>&2 "$1"
}

check_if_installed()
{
    cputype=`isainfo -k`
    modulepath="$MODDIR32/$MODNAME"
    if test "$cputype" = "amd64"; then
        modulepath="$MODDIR64/$MODNAME"
    fi
    if test -f "$modulepath"; then
        return 0
    fi

    # Check arch only if we are not stopping things (because rem_drv works for both arch.)
    if test -z "$STOPPING"; then
        # Let us go a step further and check if user has mixed up x86/amd64
        # amd64 ISA, x86 kernel module??
        if test "$cputype" = "amd64"; then
            modulepath="$MODDIR32/$MODNAME"
            if test -f "$modulepath"; then
                abort "Found 32-bit module instead of 64-bit. Please install the amd64 package!"
            fi
        else
            # x86 ISA, amd64 kernel module??
            modulepath="$MODDIR64/$MODNAME"
            if test -f "$modulepath"; then
                abort "Found 64-bit module instead of 32-bit. Please install the x86 package!"
            fi
        fi

        abort "VirtualBox Host kernel module NOT installed."
    else
        info "## VirtualBox Host kernel module NOT instaled."
        return 0
    fi
}

module_loaded()
{
    if test -f "/etc/name_to_major"; then
        loadentry=`cat /etc/name_to_major | grep $MODNAME`
    else
        loadentry=`/usr/sbin/modinfo | grep $MODNAME`
    fi
    if test -z "$loadentry"; then
        return 1
    fi
    return 0
}

vboxflt_module_loaded()
{
    if test -f "/etc/name_to_major"; then
        loadentry=`cat /etc/name_to_major | grep $FLTMODNAME`
    else
        loadentry=`/usr/sbin/modinfo | grep $FLTMODNAME`
    fi
    if test -z "$loadentry"; then
        return 1
    fi
    return 0
}

check_root()
{
    idbin=/usr/xpg4/bin/id
    if test ! -f "$idbin"; then
        found=`which id | grep "no id"`
        if test ! -z "$found"; then
            abort "Failed to find a suitable user id binary! Aborting"
        else
            idbin=$found
        fi
    fi

    if test `$idbin -u` -ne 0; then
        abort "This program must be run with administrator privileges.  Aborting"
    fi
}

start_module()
{
    if module_loaded; then
        info "VirtualBox Host kernel module already loaded."
    else
        if test -n "_HARDENED_"; then
            /usr/sbin/add_drv -m'* 0600 root sys' $MODNAME
        else
            /usr/sbin/add_drv -m'* 0666 root sys' $MODNAME
        fi
        if test ! module_loaded; then
            abort "Failed to load VirtualBox Host kernel module."
        elif test -c "/devices/pseudo/$MODNAME@0:$MODNAME"; then
            info "VirtualBox Host kernel module loaded."
        else
            abort "Aborting due to attach failure."
        fi
    fi
}

stop_module()
{
    if module_loaded; then
        /usr/sbin/rem_drv $MODNAME || abort "Failed to unload VirtualBox Host kernel module. Old one still active!"
        info "VirtualBox Host kernel module unloaded."
    elif test -z "$SILENTUNLOAD"; then
        info "VirtualBox Host kernel module not loaded."
    fi

    # check for vbi and force unload it
    vbi_mod_id=`/usr/sbin/modinfo | grep vbi | cut -f 1 -d ' ' `
    if test -n "$vbi_mod_id"; then
        /usr/sbin/modunload -i $vbi_mod_id > /dev/null 2>&1 || abort "Failed to unload VirtualBox kernel interfaces module. Old one still active!"
    fi
}

start_vboxflt()
{
    if vboxflt_module_loaded; then
        info "VirtualBox NetFilter kernel module already loaded."
    else
        /usr/sbin/add_drv -m'* 0600 root sys' $FLTMODNAME || abort "Failed to load VirtualBox Host Kernel module."
        /usr/sbin/modload -p drv/$FLTMODNAME
        if test ! vboxflt_module_loaded; then
            abort "Failed to load VirtualBox NetFilter kernel module."
        else
            info "VirtualBox NetFilter kernel module loaded."
        fi
    fi
}

stop_vboxflt()
{
    if vboxflt_module_loaded; then
        /usr/sbin/rem_drv $FLTMODNAME || abort "Failed to unload VirtualBox NetFilter module. Old one still active!"
        info "VirtualBox NetFilter kernel module unloaded."
    elif test -z "$SILENTUNLOAD"; then
        info "VirtualBox NetFilter kernel module not loaded."
    fi
}

status_module()
{
    if module_loaded; then
        info "Running."
    else
        info "Stopped."
    fi
}

stop_all_modules()
{
    stop_vboxflt
    stop_module
}

start_all_modules()
{
    start_module
    start_vboxflt
}

check_root
check_if_installed

if test "$2" = "silentunload"; then
    SILENTUNLOAD="$2"
fi

case "$1" in
stopall)
    STOPPING="stopping"
    stop_all_modules
    ;;
startall)
    start_all_modules
    ;;
start)
    start_module
    ;;
stop)
    stop_module
    ;;
status)
    status_module
    ;;
fltstart)
    start_vboxflt
    ;;
fltstop)
    stop_vboxflt
    ;;
*)
    echo "Usage: $0 {start|stop|status|fltstart|fltstop|stopall|startall}"
    exit 1
esac

exit 0

