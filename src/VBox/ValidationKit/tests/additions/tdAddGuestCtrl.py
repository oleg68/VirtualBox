#!/usr/bin/env python
# -*- coding: utf-8 -*-
# pylint: disable=C0302

"""
VirtualBox Validation Kit - Guest Control Tests.
"""

__copyright__ = \
"""
Copyright (C) 2010-2014 Oracle Corporation

This file is part of VirtualBox Open Source Edition (OSE), as
available from http://www.virtualbox.org. This file is free software;
you can redistribute it and/or modify it under the terms of the GNU
General Public License (GPL) as published by the Free Software
Foundation, in version 2 as it comes in the "COPYING" file of the
VirtualBox OSE distribution. VirtualBox OSE is distributed in the
hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

The contents of this file may alternatively be used under the terms
of the Common Development and Distribution License Version 1.0
(CDDL) only, as it comes in the "COPYING.CDDL" file of the
VirtualBox OSE distribution, in which case the provisions of the
CDDL are applicable instead of those of the GPL.

You may elect to license modified versions of this file under the
terms and conditions of either the GPL or the CDDL or both.
"""
__version__ = "$Revision$"

# Disable bitching about too many arguments per function.
# pylint: disable=R0913

## @todo Convert map() usage to a cleaner alternative Python now offers.
# pylint: disable=W0141

## @todo Convert the context/test classes into named tuples. Not in the mood right now, so
#        disabling it.
# pylint: disable=R0903

# Standard Python imports.
from array import array
import errno
import os
import random
import string # pylint: disable=W0402
import struct
import sys
import time

# Only the main script needs to modify the path.
try:    __file__
except: __file__ = sys.argv[0];
g_ksValidationKitDir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))));
sys.path.append(g_ksValidationKitDir);

# Validation Kit imports.
from testdriver import reporter;
from testdriver import base;
from testdriver import vbox;
from testdriver import vboxcon;
from testdriver import vboxwrappers;


class GuestStream(bytearray):
    """
    Class for handling a guest process input/output stream.
    """
    def appendStream(self, stream, convertTo='<b'):
        """
        Appends and converts a byte sequence to this object;
        handy for displaying a guest stream.
        """
        self.extend(struct.pack(convertTo, stream));

class tdCtxTest(object):
    """
    Provides the actual test environment. Should be kept
    as generic as possible.
    """
    def __init__(self, oSession, oTxsSession, oTestVm): # pylint: disable=W0613
        ## The desired Main API result.
        self.fRc = False;
        ## IGuest reference.
        self.oGuest = oSession.o.console.guest;
        # Rest not used (yet).

class tdCtxCreds(object):
    """
    Provides credentials to pass to the guest.
    """
    def __init__(self, sUser, sPassword, sDomain):
        self.sUser = sUser;
        self.sPassword = sPassword;
        self.sDomain = sDomain;

class tdTestGuestCtrlBase(object):
    """
    Base class for all guest control tests.
    Note: This test ASSUMES that working Guest Additions
          were installed and running on the guest to be tested.
    """
    def __init__(self):
        self.oTest  = None;
        self.oCreds = None;
        self.timeoutMS = 30 * 1000; # 30s timeout
        ## IGuestSession reference or None.
        self.oGuestSession = None;

    def setEnvironment(self, oSession, oTxsSession, oTestVm):
        """
        Sets the test environment required for this test.
        """
        self.oTest = tdCtxTest(oSession, oTxsSession, oTestVm);
        return self.oTest;

    def createSession(self, sName):
        """
        Creates (opens) a guest session.
        Returns (True, IGuestSession) on success or (False, None) on failure.
        """
        if self.oGuestSession is None:
            if sName is None:
                sName = "<untitled>";
            try:
                reporter.log('Creating session "%s" ...' % (sName,));
                self.oGuestSession = self.oTest.oGuest.createSession(self.oCreds.sUser,
                                                                     self.oCreds.sPassword,
                                                                     self.oCreds.sDomain,
                                                                     sName);
            except:
                # Just log, don't assume an error here (will be done in the main loop then).
                reporter.logXcpt('Creating a guest session "%s" failed; sUser="%s", pw="%s", sDomain="%s":'
                                 % (sName, self.oCreds.sUser, self.oCreds.sPassword, self.oCreds.sDomain));
                return (False, None);

            try:
                reporter.log('Waiting for session "%s" to start within %ldms...' % (sName, self.timeoutMS));
                fWaitFor = [ vboxcon.GuestSessionWaitForFlag_Start ];
                waitResult = self.oGuestSession.waitForArray(fWaitFor, self.timeoutMS);
                #
                # Be nice to Guest Additions < 4.3: They don't support session handling and
                # therefore return WaitFlagNotSupported.
                #
                if      waitResult != vboxcon.GuestSessionWaitResult_Start \
                    and waitResult != vboxcon.GuestSessionWaitResult_WaitFlagNotSupported:
                    # Just log, don't assume an error here (will be done in the main loop then).
                    reporter.log('Session did not start successfully, returned wait result: %ld' \
                                  % (waitResult));
                    return (False, None);
                reporter.log('Session "%s" successfully started' % (sName,));
            except:
                # Just log, don't assume an error here (will be done in the main loop then).
                reporter.logXcpt('Waiting for guest session "%s" to start failed:' % (sName));
                return (False, None);
        else:
            reporter.log('Warning: Session already set; this is probably not what you want');
        return (True, self.oGuestSession);

    def setSession(self, oGuestSession):
        """
        Sets the current guest session and closes
        an old one if necessary.
        """
        if self.oGuestSession is not None:
            self.closeSession();
        self.oGuestSession = oGuestSession;
        return self.oGuestSession;

    def closeSession(self):
        """
        Closes the guest session.
        """
        if self.oGuestSession is not None:
            sName = self.oGuestSession.name;
            try:
                reporter.log('Closing session "%s" ...' % (sName,));
                self.oGuestSession.close();
                self.oGuestSession = None;
            except:
                # Just log, don't assume an error here (will be done in the main loop then).
                reporter.logXcpt('Closing guest session "%s" failed:' % (sName,));
                return False;
        return True;

class tdTestCopyFrom(tdTestGuestCtrlBase):
    """
    Test for copying files from the guest to the host.
    """
    def __init__(self, sSrc = "", sDst = "", sUser = "", sPassword = "", aFlags = None):
        tdTestGuestCtrlBase.__init__(self);
        self.oCreds = tdCtxCreds(sUser, sPassword, sDomain = "");
        self.sSrc = sSrc;
        self.sDst = sDst;
        self.aFlags = aFlags;

class tdTestCopyTo(tdTestGuestCtrlBase):
    """
    Test for copying files from the host to the guest.
    """
    def __init__(self, sSrc = "", sDst = "", sUser = "", sPassword = "", aFlags = None):
        tdTestGuestCtrlBase.__init__(self);
        self.oCreds = tdCtxCreds(sUser, sPassword, sDomain = "");
        self.sSrc = sSrc;
        self.sDst = sDst;
        self.aFlags = aFlags;

class tdTestDirCreate(tdTestGuestCtrlBase):
    """
    Test for directoryCreate call.
    """
    def __init__(self, sDirectory = "", sUser = "", sPassword = "", fMode = 0, aFlags = None):
        tdTestGuestCtrlBase.__init__(self);
        self.oCreds = tdCtxCreds(sUser, sPassword, sDomain = "");
        self.sDirectory = sDirectory;
        self.fMode = fMode;
        self.aFlags = aFlags;

class tdTestDirCreateTemp(tdTestGuestCtrlBase):
    """
    Test for the directoryCreateTemp call.
    """
    def __init__(self, sDirectory = "", sTemplate = "", sUser = "", sPassword = "", fMode = 0, fSecure = False):
        tdTestGuestCtrlBase.__init__(self);
        self.oCreds = tdCtxCreds(sUser, sPassword, sDomain = "");
        self.sDirectory = sDirectory;
        self.sTemplate = sTemplate;
        self.fMode = fMode;
        self.fSecure = fSecure;

class tdTestDirOpen(tdTestGuestCtrlBase):
    """
    Test for the directoryOpen call.
    """
    def __init__(self, sDirectory = "", sUser = "", sPassword = "",
                 sFilter = "", aFlags = None):
        tdTestGuestCtrlBase.__init__(self);
        self.oCreds = tdCtxCreds(sUser, sPassword, sDomain = "");
        self.sDirectory = sDirectory;
        self.sFilter = sFilter;
        self.aFlags = aFlags or [];

class tdTestDirRead(tdTestDirOpen):
    """
    Test for the opening, reading and closing a certain directory.
    """
    def __init__(self, sDirectory = "", sUser = "", sPassword = "",
                 sFilter = "", aFlags = None):
        tdTestDirOpen.__init__(self, sDirectory, sUser, sPassword, sFilter, aFlags);

class tdTestExec(tdTestGuestCtrlBase):
    """
    Specifies exactly one guest control execution test.
    Has a default timeout of 5 minutes (for safety).
    """
    def __init__(self, sCmd = "", aArgs = None, aEnv = None, \
                 aFlags = None, timeoutMS = 5 * 60 * 1000, \
                 sUser = "", sPassword = "", sDomain = "", \
                 fWaitForExit = True):
        tdTestGuestCtrlBase.__init__(self);
        self.oCreds = tdCtxCreds(sUser, sPassword, sDomain);
        self.sCmd = sCmd;
        self.aArgs = aArgs;
        self.aEnv = aEnv;
        self.aFlags = aFlags or [];
        self.timeoutMS = timeoutMS;
        self.fWaitForExit = fWaitForExit;
        self.uExitStatus = 0;
        self.iExitCode = 0;
        self.cbStdOut = 0;
        self.cbStdErr = 0;
        self.sBuf = '';

class tdTestFileExists(tdTestGuestCtrlBase):
    """
    Test for the file exists API call (fileExists).
    """
    def __init__(self, sFile = "", sUser = "", sPassword = ""):
        tdTestGuestCtrlBase.__init__(self);
        self.oCreds = tdCtxCreds(sUser, sPassword, sDomain = "");
        self.sFile = sFile;

class tdTestFileRemove(tdTestGuestCtrlBase):
    """
    Test querying guest file information.
    """
    def __init__(self, sFile = "", sUser = "", sPassword = ""):
        tdTestGuestCtrlBase.__init__(self);
        self.oCreds = tdCtxCreds(sUser, sPassword, sDomain = "");
        self.sFile = sFile;

class tdTestFileStat(tdTestGuestCtrlBase):
    """
    Test querying guest file information.
    """
    def __init__(self, sFile = "", sUser = "", sPassword = "", cbSize = 0, eFileType = 0):
        tdTestGuestCtrlBase.__init__(self);
        self.oCreds = tdCtxCreds(sUser, sPassword, sDomain = "");
        self.sFile = sFile;
        self.cbSize = cbSize;
        self.eFileType = eFileType;

class tdTestFileIO(tdTestGuestCtrlBase):
    """
    Test for the IGuestFile object.
    """
    def __init__(self, sFile = "", sUser = "", sPassword = ""):
        tdTestGuestCtrlBase.__init__(self);
        self.oCreds = tdCtxCreds(sUser, sPassword, sDomain = "");
        self.sFile = sFile;

class tdTestFileQuerySize(tdTestGuestCtrlBase):
    """
    Test for the file size query API call (fileQuerySize).
    """
    def __init__(self, sFile = "", sUser = "", sPassword = ""):
        tdTestGuestCtrlBase.__init__(self);
        self.oCreds = tdCtxCreds(sUser, sPassword, sDomain = "");
        self.sFile = sFile;

class tdTestFileReadWrite(tdTestGuestCtrlBase):
    """
    Tests reading from guest files.
    """
    def __init__(self, sFile = "", sUser = "", sPassword = "",
                 sOpenMode = "r", sDisposition = "",
                 sSharingMode = "",
                 lCreationMode = 0, cbOffset = 0, cbToReadWrite = 0,
                 aBuf = None):
        tdTestGuestCtrlBase.__init__(self);
        self.oCreds = tdCtxCreds(sUser, sPassword, sDomain = "");
        self.sFile = sFile;
        self.sOpenMode = sOpenMode;
        self.sDisposition = sDisposition;
        self.sSharingMode = sSharingMode;
        self.lCreationMode = lCreationMode;
        self.cbOffset = cbOffset;
        self.cbToReadWrite = cbToReadWrite;
        self.aBuf = aBuf;

class tdTestSession(tdTestGuestCtrlBase):
    """
    Test the guest session handling.
    """
    def __init__(self, sUser = "", sPassword = "", sDomain = "", \
                 sSessionName = ""):
        tdTestGuestCtrlBase.__init__(self);
        self.sSessionName = sSessionName;
        self.oCreds = tdCtxCreds(sUser, sPassword, sDomain);

    def getSessionCount(self, oVBoxMgr):
        """
        Helper for returning the number of currently
        opened guest sessions of a VM.
        """
        if self.oTest.oGuest is None:
            return 0;
        aoSession = oVBoxMgr.getArray(self.oTest.oGuest, 'sessions')
        return len(aoSession);

class tdTestSessionEnv(tdTestGuestCtrlBase):
    """
    Test the guest session environment.
    """
    def __init__(self, sUser = "", sPassword = "", aEnv = None):
        tdTestGuestCtrlBase.__init__(self);
        self.oCreds = tdCtxCreds(sUser, sPassword, sDomain = "");
        self.aEnv = aEnv or [];

class tdTestSessionFileRefs(tdTestGuestCtrlBase):
    """
    Tests session file (IGuestFile) reference counting.
    """
    def __init__(self, cRefs = 0):
        tdTestGuestCtrlBase.__init__(self);
        self.cRefs = cRefs;

class tdTestSessionDirRefs(tdTestGuestCtrlBase):
    """
    Tests session directory (IGuestDirectory) reference counting.
    """
    def __init__(self, cRefs = 0):
        tdTestGuestCtrlBase.__init__(self);
        self.cRefs = cRefs;

class tdTestSessionProcRefs(tdTestGuestCtrlBase):
    """
    Tests session process (IGuestProcess) reference counting.
    """
    def __init__(self, cRefs = 0):
        tdTestGuestCtrlBase.__init__(self);
        self.cRefs = cRefs;

class tdTestUpdateAdditions(tdTestGuestCtrlBase):
    """
    Test updating the Guest Additions inside the guest.
    """
    def __init__(self, sSrc = "", aArgs = None, aFlags = None,
                 sUser = "", sPassword = "", sDomain = ""):
        tdTestGuestCtrlBase.__init__(self);
        self.oCreds = tdCtxCreds(sUser, sPassword, sDomain);
        self.sSrc = sSrc;
        self.aArgs = aArgs;
        self.aFlags = aFlags;

class tdTestResult(object):
    """
    Base class for test results.
    """
    def __init__(self, fRc = False):
        ## The overall test result.
        self.fRc = fRc;

class tdTestResultDirRead(tdTestResult):
    """
    Test result for reading guest directories.
    """
    def __init__(self, fRc = False,
                 numFiles = 0, numDirs = 0):
        tdTestResult.__init__(self, fRc = fRc);
        self.numFiles = numFiles;
        self.numDirs = numDirs;

class tdTestResultExec(tdTestResult):
    """
    Holds a guest process execution test result,
    including the exit code, status + aFlags.
    """
    def __init__(self, fRc = False, \
                 uExitStatus = 500, iExitCode = 0, \
                 sBuf = None, cbBuf = 0, \
                 cbStdOut = 0, cbStdErr = 0):
        tdTestResult.__init__(self);
        ## The overall test result.
        self.fRc = fRc;
        ## Process exit stuff.
        self.uExitStatus = uExitStatus;
        self.iExitCode = iExitCode;
        ## Desired buffer length returned back from stdout/stderr.
        self.cbBuf = cbBuf;
        ## Desired buffer result from stdout/stderr. Use with caution!
        self.sBuf = sBuf;
        self.cbStdOut = cbStdOut;
        self.cbStdErr = cbStdErr;

class tdTestResultFileStat(tdTestResult):
    """
    Test result for stat'ing guest files.
    """
    def __init__(self, fRc = False,
                 cbSize = 0, eFileType = 0):
        tdTestResult.__init__(self, fRc = fRc);
        self.cbSize = cbSize;
        self.eFileType = eFileType;
        ## @todo Add more information.

class tdTestResultFileReadWrite(tdTestResult):
    """
    Test result for reading + writing guest directories.
    """
    def __init__(self, fRc = False,
                 cbProcessed = 0, cbOffset = 0, aBuf = None):
        tdTestResult.__init__(self, fRc = fRc);
        self.cbProcessed = cbProcessed;
        self.cbOffset = cbOffset;
        self.aBuf = aBuf;

class tdTestResultSession(tdTestResult):
    """
    Test result for guest session counts.
    """
    def __init__(self, fRc = False, cNumSessions = 0):
        tdTestResult.__init__(self, fRc = fRc);
        self.cNumSessions = cNumSessions;

class tdTestResultSessionEnv(tdTestResult):
    """
    Test result for guest session environment tests.
    """
    def __init__(self, fRc = False, cNumVars = 0):
        tdTestResult.__init__(self, fRc = fRc);
        self.cNumVars = cNumVars;


class SubTstDrvAddGuestCtrl(base.SubTestDriverBase):
    """
    Sub-test driver for executing guest control (VBoxService, IGuest) tests.
    """

    def __init__(self, oTstDrv):
        base.SubTestDriverBase.__init__(self, 'add-guest-ctrl', oTstDrv);

        ## @todo base.TestBase.
        self.asTestsDef = \
        [
            'session_basic', 'session_env', 'session_file_ref', 'session_dir_ref', 'session_proc_ref', \
            'exec_basic', 'exec_errorlevel', 'exec_timeout', \
            'dir_create', 'dir_create_temp', 'dir_read', \
            'file_remove', 'file_stat', 'file_read', 'file_write', \
            'copy_to', 'copy_from', \
            'update_additions'
        ];
        self.asTests    = self.asTestsDef;

    def parseOption(self, asArgs, iArg):                                        # pylint: disable=R0912,R0915
        if asArgs[iArg] == '--add-guest-ctrl-tests':
            iArg += 1;
            if asArgs[iArg] == 'all': # Nice for debugging scripts.
                self.asTests = self.asTestsDef;
                return iArg + 1;

            iNext = self.oTstDrv.requireMoreArgs(1, asArgs, iArg);
            self.asTests = asArgs[iArg].split(':');
            for s in self.asTests:
                if s not in self.asTestsDef:
                    raise base.InvalidOption('The "--add-guest-ctrl-tests" value "%s" is not valid; valid values are: %s' \
                        % (s, ' '.join(self.asTestsDef)));
            return iNext;
        return iArg;

    def showUsage(self):
        base.SubTestDriverBase.showUsage(self);
        reporter.log('  --add-guest-ctrl-tests  <s1[:s2[:]]>');
        reporter.log('      Default: %s  (all)' % (':'.join(self.asTestsDef)));
        return True;

    def testIt(self, oTestVm, oSession, oTxsSession):
        """
        Executes the test.

        Returns fRc, oTxsSession.  The latter may have changed.
        """
        reporter.log("Active tests: %s" % (self.asTests,));

        # Do the testing.
        reporter.testStart('Session Basics');
        fSkip = 'session_basic' not in self.asTests;
        if fSkip == False:
            fRc, oTxsSession = self.testGuestCtrlSession(oSession, oTxsSession, oTestVm);
        reporter.testDone(fSkip);

        reporter.testStart('Session Environment');
        fSkip = 'session_env' not in self.asTests or fRc is False;
        if fSkip == False:
            fRc, oTxsSession = self.testGuestCtrlSessionEnvironment(oSession, oTxsSession, oTestVm);
        reporter.testDone(fSkip);

        reporter.testStart('Session File References');
        fSkip = 'session_file_ref' not in self.asTests;
        if fSkip == False:
            fRc, oTxsSession = self.testGuestCtrlSessionFileRefs(oSession, oTxsSession, oTestVm);
        reporter.testDone(fSkip);

        ## @todo Implement this.
        #reporter.testStart('Session Directory References');
        #fSkip = 'session_dir_ref' not in self.asTests;
        #if fSkip == False:
        #    fRc, oTxsSession = self.testGuestCtrlSessionDirRefs(oSession, oTxsSession, oTestVm);
        #reporter.testDone(fSkip);

        reporter.testStart('Session Process References');
        fSkip = 'session_proc_ref' not in self.asTests or fRc is False;
        if fSkip == False:
            fRc, oTxsSession = self.testGuestCtrlSessionProcRefs(oSession, oTxsSession, oTestVm);
        reporter.testDone(fSkip);

        reporter.testStart('Execution');
        fSkip = 'exec_basic' not in self.asTests or fRc is False;
        if fSkip == False:
            fRc, oTxsSession = self.testGuestCtrlExec(oSession, oTxsSession, oTestVm);
        reporter.testDone(fSkip);

        reporter.testStart('Execution Error Levels');
        fSkip = 'exec_errorlevel' not in self.asTests or fRc is False;
        if fSkip == False:
            fRc, oTxsSession = self.testGuestCtrlExecErrorLevel(oSession, oTxsSession, oTestVm);
        reporter.testDone(fSkip);

        reporter.testStart('Execution Timeouts');
        fSkip = 'exec_timeout' not in self.asTests or fRc is False;
        if fSkip == False:
            fRc, oTxsSession = self.testGuestCtrlExecTimeout(oSession, oTxsSession, oTestVm);
        reporter.testDone(fSkip);

        reporter.testStart('Creating directories');
        fSkip = 'dir_create' not in self.asTests or fRc is False;
        if fSkip == False:
            fRc, oTxsSession = self.testGuestCtrlDirCreate(oSession, oTxsSession, oTestVm);
        reporter.testDone(fSkip);

        reporter.testStart('Creating temporary directories');
        fSkip = 'dir_create_temp' not in self.asTests or fRc is False;
        if fSkip == False:
            fRc, oTxsSession = self.testGuestCtrlDirCreateTemp(oSession, oTxsSession, oTestVm);
        reporter.testDone(fSkip);

        reporter.testStart('Reading directories');
        fSkip = 'dir_read' not in self.asTests or fRc is False;
        if fSkip == False:
            fRc, oTxsSession = self.testGuestCtrlDirRead(oSession, oTxsSession, oTestVm);
        reporter.testDone(fSkip);

        # FIXME: Failing test.
        # reporter.testStart('Copy to guest');
        # fSkip = 'copy_to' not in self.asTests or fRc is False;
        # if fSkip == False:
        #     fRc, oTxsSession = self.testGuestCtrlCopyTo(oSession, oTxsSession, oTestVm);
        # reporter.testDone(fSkip);

        reporter.testStart('Copy from guest');
        fSkip = 'copy_from' not in self.asTests or fRc is False;
        if fSkip == False:
            fRc, oTxsSession = self.testGuestCtrlCopyFrom(oSession, oTxsSession, oTestVm);
        reporter.testDone(fSkip);

        reporter.testStart('Removing files');
        fSkip = 'file_remove' not in self.asTests or fRc is False;
        if fSkip == False:
            fRc, oTxsSession = self.testGuestCtrlFileRemove(oSession, oTxsSession, oTestVm);
        reporter.testDone(fSkip);

        reporter.testStart('Querying file information (stat)');
        fSkip = 'file_stat' not in self.asTests or fRc is False;
        if fSkip == False:
            fRc, oTxsSession = self.testGuestCtrlFileStat(oSession, oTxsSession, oTestVm);
        reporter.testDone(fSkip);

        # FIXME: Failing tests.
        # reporter.testStart('File read');
        # fSkip = 'file_read' not in self.asTests or fRc is False;
        # if fSkip == False:
        #     fRc, oTxsSession = self.testGuestCtrlFileRead(oSession, oTxsSession, oTestVm);
        # reporter.testDone(fSkip);

        # reporter.testStart('File write');
        # fSkip = 'file_write' not in self.asTests or fRc is False;
        # if fSkip == False:
        #     fRc, oTxsSession = self.testGuestCtrlFileWrite(oSession, oTxsSession, oTestVm);
        # reporter.testDone(fSkip);

        reporter.testStart('Updating Guest Additions');
        fSkip = 'update_additions' not in self.asTests or fRc is False;
        # Skip test for updating Guest Additions if we run on a too old (Windows) guest.
        fSkip = oTestVm.sKind in ('WindowsNT4', 'Windows2000', 'WindowsXP', 'Windows2003');
        if fSkip == False:
            fRc, oTxsSession = self.testGuestCtrlUpdateAdditions(oSession, oTxsSession, oTestVm);
        reporter.testDone(fSkip);

        return (fRc, oTxsSession);

    def gctrlCopyFileFrom(self, oGuestSession, sSrc, sDst, aFlags):
        """
        Helper function to copy a single file from the guest to the host.
        """
        fRc = True; # Be optimistic.
        try:
            reporter.log2('Copying guest file "%s" to host "%s"' % (sSrc, sDst));
            curProgress = oGuestSession.copyFrom(sSrc, sDst, aFlags);
            if curProgress is not None:
                oProgress = vboxwrappers.ProgressWrapper(curProgress, self.oTstDrv.oVBoxMgr, self, "gctrlCopyFrm");
                try:
                    iRc = oProgress.waitForOperation(0, fIgnoreErrors = True);
                    if iRc != 0:
                        reporter.log('Waiting for copyFrom failed');
                        fRc = False;
                except:
                    reporter.logXcpt('Copy from waiting exception for sSrc="%s", sDst="%s":' \
                                     % (sSrc, sDst));
                    fRc = False;
        except:
            # Just log, don't assume an error here (will be done in the main loop then).
            reporter.logXcpt('Copy from exception for sSrc="%s", sDst="%s":' \
                             % (sSrc, sDst));
            fRc = False;

        return fRc;

    def gctrlCopyFileTo(self, oGuestSession, sSrc, sDst, aFlags):
        """
        Helper function to copy a single file from host to the guest.
        """
        fRc = True; # Be optimistic.
        try:
            reporter.log2('Copying host file "%s" to guest "%s"' % (sSrc, sDst));
            curProgress = oGuestSession.copyTo(sSrc, sDst, aFlags);
            if curProgress is not None:
                oProgress = vboxwrappers.ProgressWrapper(curProgress, self.oTstDrv.oVBoxMgr, self, "gctrlCopyTo");
                try:
                    iRc = oProgress.waitForOperation(0, fIgnoreErrors = True);
                    if iRc != 0:
                        reporter.log('Waiting for copyTo failed');
                        fRc = False;
                except:
                    reporter.logXcpt('Copy to waiting exception for sSrc="%s", sDst="%s":' \
                                     % (sSrc, sDst));
                    fRc = False;
            else:
                reporter.error('No progress object returned');
        except:
            # Just log, don't assume an error here (will be done in the main loop then).
            reporter.logXcpt('Copy to exception for sSrc="%s", sDst="%s":' \
                             % (sSrc, sDst));
            fRc = False;

        return fRc;

    def gctrlCopyFrom(self, oTest, oRes, oGuestSession, subDir = ''): # pylint: disable=R0914
        """
        Copies files / directories to the host.
        Source always contains the absolute path, subDir all paths
        below it.
        """
        ## @todo r=bird: The use subDir and sSubDir in this method is extremely confusing!!
        #                Just follow the coding guidelines and ALWAYS use prefixes, please!
        fRc = True; # Be optimistic.

        sSrc = oTest.sSrc;
        sDst = oTest.sDst;
        aFlags = oTest.aFlags;

        if     sSrc == "" \
            or os.path.isfile(sSrc):
            return self.gctrlCopyFileFrom(oGuestSession, \
                                          sSrc, sDst, aFlags);
        sSrcAbs = sSrc;
        if subDir != "":
            sSrcAbs = os.path.join(sSrc, subDir);
        sFilter = ""; # No filter.

        sDstAbs = os.path.join(sDst, subDir);
        reporter.log2('Copying guest directory "%s" to host "%s"' % (sSrcAbs, sDstAbs));

        try:
            #reporter.log2('Directory="%s", filter="%s", aFlags="%s"' % (sCurDir, sFilter, aFlags));
            oCurDir = oGuestSession.directoryOpen(sSrcAbs, sFilter, aFlags);
            while fRc:
                try:
                    oFsObjInfo = oCurDir.read();
                    if     oFsObjInfo.name == "." \
                        or oFsObjInfo.name == "..":
                        #reporter.log2('\tSkipping "%s"' % (oFsObjInfo.name,));
                        continue; # Skip "." and ".." entries.
                    if oFsObjInfo.type is vboxcon.FsObjType_Directory:
                        #reporter.log2('\tDirectory "%s"' % (oFsObjInfo.name,));
                        sDirCreate = sDst;
                        if subDir != "":
                            sDirCreate = os.path.join(sDirCreate, subDir);
                        sDirCreate = os.path.join(sDirCreate, oFsObjInfo.name);
                        try:
                            reporter.log2('\tCreating directory "%s"' % (sDirCreate,));
                            os.makedirs(sDirCreate);
                            sSubDir = oFsObjInfo.name;
                            if subDir != "":
                                sSubDir = os.path.join(subDir, oFsObjInfo.name);
                            self.gctrlCopyFrom(oTest, oRes, oGuestSession, sSubDir);
                        except (OSError) as e:
                            if e.errno == errno.EEXIST:
                                pass;
                            else:
                                # Just log, don't assume an error here (will be done in the main loop then).
                                reporter.logXcpt('\tDirectory creation exception for directory="%s":' % (sSubDir,));
                                raise;
                    elif oFsObjInfo.type is vboxcon.FsObjType_File:
                        #reporter.log2('\tFile "%s"' % (oFsObjInfo.name,));
                        sSourceFile = os.path.join(sSrcAbs, oFsObjInfo.name);
                        sDestFile = os.path.join(sDstAbs, oFsObjInfo.name);
                        self.gctrlCopyFileFrom(oGuestSession, sSourceFile, sDestFile, aFlags);
                    elif oFsObjInfo.type is vboxcon.FsObjType_Symlink:
                        #reporter.log2('\tSymlink "%s" -- not tested yet' % oFsObjInfo.name);
                        pass;
                    else:
                        reporter.error('\tDirectory "%s" contains invalid directory entry "%s" (%d)' \
                                       % (sSubDir, oFsObjInfo.name, oFsObjInfo.type));
                        fRc = False;
                except Exception, oXcpt:
                    # No necessarily an error -- could be VBOX_E_OBJECT_NOT_FOUND. See reference.
                    if vbox.ComError.equal(oXcpt, vbox.ComError.VBOX_E_OBJECT_NOT_FOUND):
                        #reporter.log2('\tNo more directory entries for "%s"' % (sCurDir,));
                        break
                    # Just log, don't assume an error here (will be done in the main loop then).
                    reporter.logXcpt('\tDirectory read exception for directory="%s":' % (sSrcAbs,));
                    fRc = False;
                    break;
            oCurDir.close();
        except:
            # Just log, don't assume an error here (will be done in the main loop then).
            reporter.logXcpt('\tDirectory open exception for directory="%s":' % (sSrcAbs,));
            fRc = False;

        return fRc;

    def gctrlCopyTo(self, oTest, oGuestSession, subDir = ''): # pylint: disable=R0914
        """
        Copies files / directories to the guest.
        Source always contains the absolute path,
        subDir all paths below it.
        """
        fRc = True; # Be optimistic.

        sSrc = oTest.sSrc;
        sDst = oTest.sDst;
        aFlags = oTest.aFlags;

        reporter.log2('sSrc=%s, sDst=%s, aFlags=%s' % (sSrc, sDst, aFlags));

        sSrcAbs = sSrc;
        if subDir != "":
            sSrcAbs = os.path.join(sSrc, subDir);

        # Note: Current test might want to test copying empty sources
        if     not os.path.exists(sSrcAbs) \
            or os.path.isfile(sSrcAbs):
            return self.gctrlCopyFileTo(oGuestSession, \
                                        sSrcAbs, sDst, aFlags);

        sDstAbs = os.path.join(sDst, subDir);
        reporter.log2('Copying host directory "%s" to guest "%s"' % (sSrcAbs, sDstAbs));

        try:
            # Note: Symlinks intentionally not considered here.
            for (sDirCurrent, oDirs, oFiles) in os.walk(sSrcAbs, topdown=True):
                for sFile in oFiles:
                    sCurFile = os.path.join(sDirCurrent, sFile);
                    reporter.log2('Processing host file "%s"' % (sCurFile,));
                    sFileDest = os.path.join(sDstAbs, os.path.relpath(sCurFile, sSrcAbs));
                    reporter.log2('Copying file to "%s"' % (sFileDest,));
                    fRc = self.gctrlCopyFileTo(oGuestSession, \
                                               sCurFile, sFileDest, aFlags);
                    if fRc is False:
                        break;

                if fRc is False:
                    break;

                for sSubDir in oDirs:
                    sCurDir = os.path.join(sDirCurrent, sSubDir);
                    reporter.log2('Processing host dir "%s"' % (sCurDir,));
                    sDirDest = os.path.join(sDstAbs, os.path.relpath(sCurDir, sSrcAbs));
                    reporter.log2('Creating guest dir "%s"' % (sDirDest,));
                    oGuestSession.directoryCreate(sDirDest, \
                                                  700, [ vboxcon.DirectoryCreateFlag_Parents ]);
                    if fRc is False:
                        break;

                if fRc is False:
                    break;
        except:
            # Just log, don't assume an error here (will be done in the main loop then).
            reporter.logXcpt('Copy to exception for sSrc="%s", sDst="%s":' \
                             % (sSrcAbs, sDst));
            return False;

        return fRc;

    def gctrlCreateDir(self, oTest, oRes, oGuestSession):
        """
        Helper function to create a guest directory specified in
        the current test.
        """
        fRc = True; # Be optimistic.
        reporter.log2('Creating directory "%s"' % (oTest.sDirectory,));

        try:
            oGuestSession.directoryCreate(oTest.sDirectory, \
                                          oTest.fMode, oTest.aFlags);
            fDirExists = oGuestSession.directoryExists(oTest.sDirectory);
            if      fDirExists is False \
                and oRes.fRc is True:
                # Directory does not exist but we want it to.
                fRc = False;
        except:
            reporter.logXcpt('Directory create exception for directory "%s":' % (oTest.sDirectory,));
            if oRes.fRc is True:
                # Just log, don't assume an error here (will be done in the main loop then).
                fRc = False;
            # Directory creation failed, which was the expected result.

        return fRc;

    def gctrlReadDir(self, oTest, oRes, oGuestSession, subDir = ''): # pylint: disable=R0914
        """
        Helper function to read a guest directory specified in
        the current test.
        """
        sDir     = oTest.sDirectory;
        sFilter  = oTest.sFilter;
        aFlags   = oTest.aFlags;

        fRc = True; # Be optimistic.
        cDirs = 0;  # Number of directories read.
        cFiles = 0; # Number of files read.

        try:
            sCurDir = os.path.join(sDir, subDir);
            #reporter.log2('Directory="%s", filter="%s", aFlags="%s"' % (sCurDir, sFilter, aFlags));
            oCurDir = oGuestSession.directoryOpen(sCurDir, sFilter, aFlags);
            while fRc:
                try:
                    oFsObjInfo = oCurDir.read();
                    if     oFsObjInfo.name == "." \
                        or oFsObjInfo.name == "..":
                        #reporter.log2('\tSkipping "%s"' % oFsObjInfo.name);
                        continue; # Skip "." and ".." entries.
                    if oFsObjInfo.type is vboxcon.FsObjType_Directory:
                        #reporter.log2('\tDirectory "%s"' % oFsObjInfo.name);
                        cDirs += 1;
                        sSubDir = oFsObjInfo.name;
                        if subDir != "":
                            sSubDir = os.path.join(subDir, oFsObjInfo.name);
                        fRc, cSubDirs, cSubFiles = self.gctrlReadDir(oTest, oRes, oGuestSession, sSubDir);
                        cDirs += cSubDirs;
                        cFiles += cSubFiles;
                    elif oFsObjInfo.type is vboxcon.FsObjType_File:
                        #reporter.log2('\tFile "%s"' % oFsObjInfo.name);
                        cFiles += 1;
                    elif oFsObjInfo.type is vboxcon.FsObjType_Symlink:
                        #reporter.log2('\tSymlink "%s" -- not tested yet' % oFsObjInfo.name);
                        pass;
                    else:
                        reporter.error('\tDirectory "%s" contains invalid directory entry "%s" (type %d)' % \
                                       (sCurDir, oFsObjInfo.name, oFsObjInfo.type));
                        fRc = False;
                except Exception, oXcpt:
                    # No necessarily an error -- could be VBOX_E_OBJECT_NOT_FOUND. See reference.
                    if vbox.ComError.equal(oXcpt, vbox.ComError.VBOX_E_OBJECT_NOT_FOUND):
                        #reporter.log2('\tNo more directory entries for "%s"' % (sCurDir,));
                        break
                    # Just log, don't assume an error here (will be done in the main loop then).
                    reporter.logXcpt('\tDirectory open exception for directory="%s":' % (sCurDir,));
                    fRc = False;
                    break;
            oCurDir.close();
        except:
            # Just log, don't assume an error here (will be done in the main loop then).
            reporter.logXcpt('\tDirectory open exception for directory="%s":' % (sCurDir,));
            fRc = False;

        return (fRc, cDirs, cFiles);

    def gctrlExecDoTest(self, i, oTest, oRes, oGuestSession):
        """
        Wrapper function around gctrlExecute to provide more sanity checking
        when needed in actual execution tests.
        """
        reporter.log('Testing #%d, cmd="%s" ...' % (i, oTest.sCmd));
        fRc = self.gctrlExecute(oTest, oGuestSession);
        if fRc is oRes.fRc:
            if fRc is True:
                # Compare exit status / code on successful process execution.
                if     oTest.uExitStatus != oRes.uExitStatus \
                    or oTest.iExitCode   != oRes.iExitCode:
                    reporter.error('Test #%d failed: Got exit status + code %d,%d, expected %d,%d' % \
                                   (i, oTest.uExitStatus, oTest.iExitCode, \
                                       oRes.uExitStatus,  oRes.iExitCode));
                    return False;
            if fRc is True:
                # Compare test / result buffers on successful process execution.
                if      oTest.sBuf is not None \
                    and oRes.sBuf is not None:
                    if bytes(oTest.sBuf) != bytes(oRes.sBuf):
                        reporter.error('Test #%d failed: Got buffer\n%s (%ld bytes), expected\n%s (%ld bytes)' %
                                       (i, map(hex, map(ord, oTest.sBuf)), len(oTest.sBuf), \
                                           map(hex, map(ord, oRes.sBuf)), len(oRes.sBuf)));
                        return False;
                    else:
                        reporter.log2('Test #%d passed: Buffers match (%ld bytes)' % (i, len(oRes.sBuf)));
                elif     oRes.sBuf is not None \
                     and len(oRes.sBuf):
                    reporter.error('Test #%d failed: Got no buffer data, expected\n%s (%dbytes)' %
                                   (i, map(hex, map(ord, oRes.sBuf)), len(oRes.sBuf)));
                    return False;
                elif     oRes.cbStdOut > 0 \
                     and oRes.cbStdOut != oTest.cbStdOut:
                    reporter.error('Test #%d failed: Got %ld stdout data, expected %ld'
                                   % (i, oTest.cbStdOut, oRes.cbStdOut));
                    return False;
        else:
            reporter.error('Test #%d failed: Got %s, expected %s' % (i, fRc, oRes.fRc));
            return False;
        return True;

    def gctrlExecute(self, oTest, oGuestSession):
        """
        Helper function to execute a program on a guest, specified in
        the current test.
        """
        fRc = True; # Be optimistic.

        ## @todo Compare execution timeouts!
        #tsStart = base.timestampMilli();

        reporter.log2('Using session user=%s, sDomain=%s, session name=%s, session timeout=%ld' \
                      % (oGuestSession.user, oGuestSession.domain, \
                         oGuestSession.name, oGuestSession.timeout));
        reporter.log2('Executing cmd=%s, aFlags=%s, timeout=%ld, args=%s, env=%s' \
                      % (oTest.sCmd, oTest.aFlags, oTest.timeoutMS, \
                         oTest.aArgs, oTest.aEnv));
        try:
            curProc = oGuestSession.processCreate(oTest.sCmd, \
                                                  oTest.aArgs, oTest.aEnv, \
                                                  oTest.aFlags, oTest.timeoutMS);
            if curProc is not None:
                reporter.log2('Process start requested, waiting for start (%ldms) ...' % (oTest.timeoutMS,));
                fWaitFor = [ vboxcon.ProcessWaitForFlag_Start ];
                waitResult = curProc.waitForArray(fWaitFor, oTest.timeoutMS);
                reporter.log2('Wait result returned: %d, current process status is: %ld' % (waitResult, curProc.status));

                if curProc.status == vboxcon.ProcessStatus_Started:
                    fWaitFor = [ vboxcon.ProcessWaitForFlag_Terminate ];
                    if vboxcon.ProcessCreateFlag_WaitForStdOut in oTest.aFlags:
                        fWaitFor.append(vboxcon.ProcessWaitForFlag_StdOut);
                    if vboxcon.ProcessCreateFlag_WaitForStdErr in oTest.aFlags:
                        fWaitFor.append(vboxcon.ProcessWaitForFlag_StdErr);
                    ## @todo Add vboxcon.ProcessWaitForFlag_StdIn.
                    reporter.log2('Process (PID %ld) started, waiting for termination (%dms), waitFlags=%s ...' \
                                  % (curProc.PID, oTest.timeoutMS, fWaitFor));
                    while True:
                        waitResult = curProc.waitForArray(fWaitFor, oTest.timeoutMS);
                        reporter.log2('Wait returned: %d' % (waitResult,));
                        try:
                            # Try stdout.
                            if     waitResult == vboxcon.ProcessWaitResult_StdOut \
                                or waitResult == vboxcon.ProcessWaitResult_WaitFlagNotSupported:
                                reporter.log2('Reading stdout ...');
                                buf = curProc.Read(1, 64 * 1024, oTest.timeoutMS);
                                if len(buf):
                                    reporter.log2('Process (PID %ld) got %ld bytes of stdout data' % (curProc.PID, len(buf)));
                                    oTest.cbStdOut += len(buf);
                                    oTest.sBuf = buf; # Appending does *not* work atm, so just assign it. No time now.
                            # Try stderr.
                            if     waitResult == vboxcon.ProcessWaitResult_StdErr \
                                or waitResult == vboxcon.ProcessWaitResult_WaitFlagNotSupported:
                                reporter.log2('Reading stderr ...');
                                buf = curProc.Read(2, 64 * 1024, oTest.timeoutMS);
                                if len(buf):
                                    reporter.log2('Process (PID %ld) got %ld bytes of stderr data' % (curProc.PID, len(buf)));
                                    oTest.cbStdErr += len(buf);
                                    oTest.sBuf = buf; # Appending does *not* work atm, so just assign it. No time now.
                            # Use stdin.
                            if     waitResult == vboxcon.ProcessWaitResult_StdIn \
                                or waitResult == vboxcon.ProcessWaitResult_WaitFlagNotSupported:
                                pass; #reporter.log2('Process (PID %ld) needs stdin data' % (curProc.pid,));
                            # Termination or error?
                            if     waitResult == vboxcon.ProcessWaitResult_Terminate \
                                or waitResult == vboxcon.ProcessWaitResult_Error \
                                or waitResult == vboxcon.ProcessWaitResult_Timeout:
                                reporter.log2('Process (PID %ld) reported terminate/error/timeout: %ld, status: %ld' \
                                              % (curProc.PID, waitResult, curProc.status));
                                break;
                        except:
                            # Just skip reads which returned nothing.
                            pass;
                    reporter.log2('Final process status (PID %ld) is: %ld' % (curProc.PID, curProc.status));
                    reporter.log2('Process (PID %ld) %ld stdout, %ld stderr' % (curProc.PID, oTest.cbStdOut, oTest.cbStdErr));
            oTest.uExitStatus = curProc.status;
            oTest.iExitCode = curProc.exitCode;
            reporter.log2('Process (PID %ld) has exit code: %ld' % (curProc.PID, oTest.iExitCode));
        except KeyboardInterrupt:
            reporter.error('Process (PID %ld) execution interrupted' % (curProc.PID,));
            if curProc is not None:
                curProc.close();
        except:
            # Just log, don't assume an error here (will be done in the main loop then).
            reporter.logXcpt('Execution exception for command "%s":' % (oTest.sCmd,));
            fRc = False;

        return fRc;

    def testGuestCtrlSessionEnvironment(self, oSession, oTxsSession, oTestVm): # pylint: disable=R0914
        """
        Tests the guest session environment.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
        else:
            sUser = "vbox";
        sPassword = "password";

        aaTests = [
            # No environment set.
            [ tdTestSessionEnv(sUser = sUser, sPassword = sPassword),
              tdTestResultSessionEnv(fRc = False) ],
            # Invalid stuff.
            [ tdTestSessionEnv(sUser = sUser, sPassword = sPassword, aEnv = [ '=FOO' ]),
              tdTestResultSessionEnv(fRc = False) ],
            [ tdTestSessionEnv(sUser = sUser, sPassword = sPassword, aEnv = [ '====' ]),
              tdTestResultSessionEnv(fRc = False) ],
            [ tdTestSessionEnv(sUser = sUser, sPassword = sPassword, aEnv = [ '=BAR' ]),
              tdTestResultSessionEnv(fRc = False) ],
            [ tdTestSessionEnv(sUser = sUser, sPassword = sPassword, aEnv = [ u'ß$%ß&' ]),
              tdTestResultSessionEnv(fRc = False) ],
            # Key only.
            [ tdTestSessionEnv(sUser = sUser, sPassword = sPassword, aEnv = [ 'FOO=' ]),
              tdTestResultSessionEnv(fRc = True, cNumVars = 1) ],
            # Values.
            [ tdTestSessionEnv(sUser = sUser, sPassword = sPassword, aEnv = [ 'FOO' ]),
              tdTestResultSessionEnv(fRc = True, cNumVars = 1) ],
            [ tdTestSessionEnv(sUser = sUser, sPassword = sPassword, aEnv = [ 'FOO=BAR' ]),
              tdTestResultSessionEnv(fRc = True, cNumVars = 1) ],
            [ tdTestSessionEnv(sUser = sUser, sPassword = sPassword, aEnv = [ 'FOO=BAR', 'BAR=BAZ' ]),
              tdTestResultSessionEnv(fRc = True, cNumVars = 2) ],
            # A bit more weird keys/values.
            [ tdTestSessionEnv(sUser = sUser, sPassword = sPassword, aEnv = [ '$$$=' ]),
              tdTestResultSessionEnv(fRc = True, cNumVars = 1) ],
            [ tdTestSessionEnv(sUser = sUser, sPassword = sPassword, aEnv = [ '$$$=%%%' ]),
              tdTestResultSessionEnv(fRc = True, cNumVars = 1) ],
            # Same stuff.
            [ tdTestSessionEnv(sUser = sUser, sPassword = sPassword, aEnv = [ 'FOO=BAR', 'FOO=BAR' ]),
              tdTestResultSessionEnv(fRc = True, cNumVars = 1) ]
        ];

        # Parameters.
        fRc = True;
        for (i, aTest) in enumerate(aaTests):
            curTest = aTest[0]; # tdTestExec, use an index, later.
            curRes  = aTest[1]; # tdTestResult
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            reporter.log('Testing #%d, user="%s", sPassword="%s", env="%s" (%d)...' \
                         % (i, curTest.oCreds.sUser, curTest.oCreds.sPassword, curTest.aEnv, len(curTest.aEnv)));
            curGuestSessionName = 'testGuestCtrlSessionEnvironment: Test #%d' % (i,);
            fRc2, curGuestSession = curTest.createSession(curGuestSessionName);
            if fRc2 is not True:
                reporter.error('Test #%d failed: Session creation failed: Got %s, expected True' % (i, fRc2));
                fRc = False;
                break;
            # Make sure environment is empty.
            curEnv = self.oTstDrv.oVBoxMgr.getArray(curGuestSession, 'environment');
            reporter.log2('Test #%d: Environment initially has %d elements' % (i, len(curEnv)));
            if len(curEnv) != 0:
                reporter.error('Test #%d failed: Initial session environment has %d vars, expected 0' % (i, len(curEnv)));
                fRc = False;
                break;
            try:
                for (_, aEnv) in enumerate(curTest.aEnv): # Enumerate only will work with a sequence (e.g > 1 entries).
                    aElems = aEnv.split('=');
                    strKey = '';
                    strValue = '';
                    if len(aElems) > 0:
                        strKey = aElems[0];
                    if len(aElems) == 2:
                        strValue = aElems[1];
                    reporter.log2('Test #%d: Single key="%s", value="%s" (%d) ...' \
                                  % (i, strKey, strValue, len(aElems)));
                    try:
                        curGuestSession.environmentSet(strKey, strValue); # No return (e.g. boolean) value available thru wrapper.
                    except:
                        # Setting environment variables might fail (e.g. if empty name specified). Check.
                        reporter.logXcpt('Test #%d failed: Setting environment variable failed:' % (i,));
                        curEnv = self.oTstDrv.oVBoxMgr.getArray(curGuestSession, 'environment');
                        if len(curEnv) is not curRes.cNumVars:
                            reporter.error('Test #%d failed: Session environment has %d vars, expected %d' \
                                           % (i, len(curEnv), curRes.cNumVars));
                            fRc = False;
                            break;
                        else:
                            reporter.log('Test #%d: API reported an error (single), good' % (i,));
                    reporter.log2('Getting key="%s" ...' % (strKey,));
                    try:
                        strValue2 = curGuestSession.environmentGet(strKey);
                        if      strKey.isalnum() \
                            and strValue != strValue2:
                            reporter.error('Test #%d failed: Got environment variable "%s", expected "%s" (key: "%s")' \
                                           % (i, strValue2, strValue, strKey));
                            fRc = False;
                            break;
                        # Getting back an empty value when specifying an invalid key is fine.
                        reporter.log2('Got key "%s=%s"' % (strKey, strValue2));
                    except UnicodeDecodeError: # Might happen on unusal values, fine.
                        if strValue != strValue2:
                            reporter.error('Test #%d failed: Got (undecoded) environment variable "%s", ' \
                                           'expected "%s" (key: "%s")' \
                                           % (i, strValue2, strValue, strKey));
                            fRc = False;
                            break;
                    except:
                        if     strKey == "" \
                            or not strKey.isalnum():
                            reporter.log('Test #%d: API reported an error (invalid key "%s"), good' % (i, strKey));
                        else:
                            reporter.errorXcpt('Test #%d failed: Getting environment variable:' % (i));
                if fRc is False:
                    continue;
                # Set the same stuff again, this time all at once using the array.
                if len(curTest.aEnv):
                    reporter.log('Test #%d: Array %s (%d)' % (i, curTest.aEnv, len(curTest.aEnv)));
                    try:
                        ## @todo No return (e.g. boolean) value available thru wrapper.
                        #curGuestSession.environmentSetArray(curTest.aEnv);
                        pass;
                    except:
                        # Setting environment variables might fail (e.g. if empty name specified). Check.
                        curEnv = self.oTstDrv.oVBoxMgr.getArray(curGuestSession, 'environment');
                        if len(curEnv) is not curRes.cNumVars:
                            reporter.error('Test #%d failed: Session environment has %d vars, expected %d (array)' \
                                           % (i, len(curEnv), curRes.cNumVars));
                            fRc = False;
                            break;
                        else:
                            reporter.log('Test #%d: API reported an error (array), good' % (i,));
                ## @todo Get current system environment and add it to curRes.cNumVars before comparing!
                reporter.log('Test #%d: Environment size' % (i,));
                curEnv = self.oTstDrv.oVBoxMgr.getArray(curGuestSession, 'environment');
                reporter.log2('Test #%d: Environment (%d) -> %s' % (i, len(curEnv), curEnv));
                if len(curEnv) != curRes.cNumVars:
                    reporter.error('Test #%d failed: Session environment has %d vars (%s), expected %d' \
                                   % (i, len(curEnv), curEnv, curRes.cNumVars));
                    fRc = False;
                    break;
                curGuestSession.environmentClear(); # No return (e.g. boolean) value available thru wrapper.
                curEnv = self.oTstDrv.oVBoxMgr.getArray(curGuestSession, 'environment');
                if len(curEnv) is not 0:
                    reporter.error('Test #%d failed: Session environment has %d vars, expected 0');
                    fRc = False;
                    break;
            except:
                reporter.errorXcpt('Test #%d failed:' % (i,));

            fRc2 = curTest.closeSession();
            if fRc2 is False:
                reporter.error('Test #%d failed: Session could not be closed' % (i,));
                fRc = False;
                break;

        return (fRc, oTxsSession);

    def testGuestCtrlSession(self, oSession, oTxsSession, oTestVm): # pylint: disable=R0914
        """
        Tests the guest session handling.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
        else:
            sUser = "vbox";
        sPassword = "password";

        aaTests = [
            # Invalid parameters.
            [ tdTestSession(),
              tdTestResultSession(fRc = False) ],
            [ tdTestSession(sUser = ''),
              tdTestResultSession(fRc = False) ],
            [ tdTestSession(sPassword = 'bar'),
              tdTestResultSession(fRc = False) ],
            [ tdTestSession(sDomain = 'boo'),
              tdTestResultSession(fRc = False) ],
            [ tdTestSession(sPassword = 'bar', sDomain = 'boo'),
              tdTestResultSession(fRc = False) ],
            # User account without a passwort - forbidden.
            [ tdTestSession(sUser = sUser),
              tdTestResultSession(fRc = False) ],
            # Wrong credentials.
            # Note: On Guest Additions < 4.3 this always succeeds because these don't
            #       support creating dedicated sessions. Instead, guest process creation
            #       then will fail. See note below.
            [ tdTestSession(sUser = 'foo', sPassword = 'bar', sDomain = 'boo'),
              tdTestResultSession(fRc = False) ],
            # Correct credentials.
            [ tdTestSession(sUser = sUser, sPassword = sPassword),
              tdTestResultSession(fRc = True, cNumSessions = 1) ]
        ];

        # Parameters.
        fRc = True;
        for (i, aTest) in enumerate(aaTests):
            curTest = aTest[0]; # tdTestSession, use an index, later.
            curRes  = aTest[1]; # tdTestResult
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            reporter.log('Testing #%d, user="%s", sPassword="%s", sDomain="%s" ...' \
                         % (i, curTest.oCreds.sUser, curTest.oCreds.sPassword, curTest.oCreds.sDomain));
            curGuestSessionName = 'testGuestCtrlSession: Test #%d' % (i);
            fRc2, curGuestSession = curTest.createSession(curGuestSessionName);
            # See note about < 4.3 Guest Additions above.
            if      curGuestSession is not None \
                and curGuestSession.protocolVersion >= 2 \
                and fRc2 is not curRes.fRc:
                reporter.error('Test #%d failed: Session creation failed: Got %s, expected %s' \
                               % (i, fRc2, curRes.fRc));
                fRc = False;
            if fRc2:
                # On Guest Additions < 4.3 getSessionCount() always will return 1, so skip the
                # check then.
                if curGuestSession.protocolVersion >= 2:
                    curSessionCount = curTest.getSessionCount(self.oTstDrv.oVBoxMgr);
                    if curSessionCount is not curRes.cNumSessions:
                        reporter.error('Test #%d failed: Session count does not match: Got %d, expected %d' \
                                       % (i, curSessionCount, curRes.cNumSessions));
                        fRc = False;
                        break;
                if      curGuestSession is not None \
                    and curGuestSession.name != curGuestSessionName:
                    reporter.error('Test #%d failed: Session name does not match: Got "%s", expected "%s"' \
                                   % (i, curGuestSession.name, curGuestSessionName));
                    fRc = False;
                    break;
            fRc2 = curTest.closeSession();
            if fRc2 is False:
                reporter.error('Test #%d failed: Session could not be closed' % (i,));
                fRc = False;
                break;

        if fRc is False:
            return (False, oTxsSession);

        # Multiple sessions.
        iMaxGuestSessions = 31; # Maximum number of concurrent guest session allowed.
                                # Actually, this is 32, but we don't test session 0.
        multiSession = {};
        reporter.log2('Opening multiple guest tsessions at once ...');
        for i in range(iMaxGuestSessions + 1):
            multiSession[i] = tdTestSession(sUser = sUser, sPassword = sPassword, sSessionName = 'MultiSession #%d' % (i,));
            multiSession[i].setEnvironment(oSession, oTxsSession, oTestVm);
            curSessionCount = multiSession[i].getSessionCount(self.oTstDrv.oVBoxMgr);
            reporter.log2('MultiSession test #%d count is %d' % (i, curSessionCount));
            if curSessionCount is not i:
                reporter.error('MultiSession count #%d must be %d, got %d' % (i, i, curSessionCount));
                fRc = False;
                break;
            fRc2, _ = multiSession[i].createSession('MultiSession #%d' % (i,));
            if fRc2 is not True:
                if i < iMaxGuestSessions:
                    reporter.error('MultiSession #%d test failed' % (i,));
                    fRc = False;
                else:
                    reporter.log('MultiSession #%d exceeded concurrent guest session count, good' % (i,));
                break;

        curSessionCount = multiSession[i].getSessionCount(self.oTstDrv.oVBoxMgr);
        if curSessionCount is not iMaxGuestSessions:
            reporter.error('Final MultiSession count must be %d, got %d'
                           % (iMaxGuestSessions, curSessionCount));
            return (False, oTxsSession);

        reporter.log2('Closing MultiSessions ...');
        iLastSession = iMaxGuestSessions - 1;
        for i in range(iLastSession): # Close all but the last opened session.
            fRc2 = multiSession[i].closeSession();
            reporter.log2('MultiSession #%d count is %d' % (i, multiSession[i].getSessionCount(self.oTstDrv.oVBoxMgr),));
            if fRc2 is False:
                reporter.error('Closing MultiSession #%d failed' % (i,));
                fRc = False;
                break;
        curSessionCount = multiSession[i].getSessionCount(self.oTstDrv.oVBoxMgr);
        if curSessionCount is not 1:
            reporter.error('Final MultiSession count #2 must be 1, got %d' % (curSessionCount,));
            fRc = False;

        try:
            # Make sure that accessing the first opened guest session does not work anymore because we just removed (closed) it.
            curSessionName = multiSession[0].oGuestSession.name;
            reporter.error('Accessing first removed MultiSession should not be possible, got name="%s"' % (curSessionName,));
            fRc = False;
        except:
            reporter.logXcpt('Could not access first removed MultiSession object, good:');

        try:
            # Try Accessing last opened session which did not get removed yet.
            curSessionName = multiSession[iLastSession].oGuestSession.name;
            reporter.log('Accessing last standing MultiSession worked, got name="%s"' % (curSessionName,));
            multiSession[iLastSession].closeSession();
            curSessionCount = multiSession[i].getSessionCount(self.oTstDrv.oVBoxMgr);
            if curSessionCount is not 0:
                reporter.error('Final MultiSession count #3 must be 0, got %d' % (curSessionCount,));
                fRc = False;
        except:
            reporter.logXcpt('Could not access last standing MultiSession object:');
            fRc = False;

        ## @todo Test session timeouts.

        return (fRc, oTxsSession);

    def testGuestCtrlSessionFileRefs(self, oSession, oTxsSession, oTestVm): # pylint: disable=R0914
        """
        Tests the guest session file reference handling.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
            sPassword = "password";
            sDomain = "";
            sFile = "C:\\windows\\system32\\kernel32.dll";

        # Number of stale guest files to create.
        cStaleFiles = 10;

        fRc = True;
        try:
            oGuest = oSession.o.console.guest;
            oGuestSession = oGuest.createSession(sUser, sPassword, sDomain, \
                                                 "testGuestCtrlSessionFileRefs");
            fWaitFor = [ vboxcon.GuestSessionWaitForFlag_Start ];
            waitResult = oGuestSession.waitForArray(fWaitFor, 30 * 1000);
            #
            # Be nice to Guest Additions < 4.3: They don't support session handling and
            # therefore return WaitFlagNotSupported.
            #
            if      waitResult != vboxcon.GuestSessionWaitResult_Start \
                and waitResult != vboxcon.GuestSessionWaitResult_WaitFlagNotSupported:
                # Just log, don't assume an error here (will be done in the main loop then).
                reporter.log('Session did not start successfully, returned wait result: %ld' \
                              % (waitResult));
                return (False, oTxsSession);
            reporter.log('Session successfully started');

            #
            # Open guest files and "forget" them (stale entries).
            # For them we don't have any references anymore intentionally.
            #
            reporter.log2('Opening stale files');
            for i in range(0, cStaleFiles):
                try:
                    oGuestSession.fileOpen(sFile, "r", "oe", 0);
                    # Note: Use a timeout in the call above for not letting the stale processes
                    #       hanging around forever.  This can happen if the installed Guest Additions
                    #       do not support terminating guest processes.
                except:
                    reporter.errorXcpt('Opening stale file #%ld failed:' % (i,));
                    fRc = False;
                    break;

            if fRc:
                cFiles = len(self.oTstDrv.oVBoxMgr.getArray(oGuestSession, 'files'));
                if cFiles != cStaleFiles:
                    reporter.error('Test failed: Got %ld stale files, expected %ld' % (cFiles, cStaleFiles));
                    fRc = False;

            if fRc:
                #
                # Open non-stale files and close them again.
                #
                reporter.log2('Opening non-stale files');
                aaFiles = [];
                for i in range(0, cStaleFiles):
                    try:
                        oCurFile = oGuestSession.fileOpen(sFile, "r", "oe", 0);
                        aaFiles.append(oCurFile);
                    except:
                        reporter.errorXcpt('Opening non-stale file #%ld failed:' % (i,));
                        fRc = False;
                        break;
            if fRc:
                cFiles = len(self.oTstDrv.oVBoxMgr.getArray(oGuestSession, 'files'));
                if cFiles != cStaleFiles * 2:
                    reporter.error('Test failed: Got %ld total files, expected %ld' % (cFiles, cStaleFiles * 2));
                    fRc = False;
            if fRc:
                reporter.log2('Closing all non-stale files again ...');
                for i in range(0, cStaleFiles):
                    try:
                        aaFiles[i].close();
                    except:
                        reporter.errorXcpt('Waiting for non-stale file #%ld failed:' % (i,));
                        fRc = False;
                        break;
                cFiles = len(self.oTstDrv.oVBoxMgr.getArray(oGuestSession, 'files'));
                # Here we count the stale files (that is, files we don't have a reference
                # anymore for) and the opened and then closed non-stale files (that we still keep
                # a reference in aaFiles[] for).
                if cFiles != cStaleFiles:
                    reporter.error('Test failed: Got %ld total files, expected %ld' \
                                   % (cFiles, cStaleFiles));
                    fRc = False;
                if fRc:
                    #
                    # Check if all (referenced) non-stale files now are in "closed" state.
                    #
                    reporter.log2('Checking statuses of all non-stale files ...');
                    for i in range(0, cStaleFiles):
                        try:
                            curFilesStatus = aaFiles[i].status;
                            if curFilesStatus != vboxcon.FileStatus_Closed:
                                reporter.error('Test failed: Non-stale file #%ld has status %ld, expected %ld' \
                                       % (i, curFilesStatus, vboxcon.FileStatus_Closed));
                                fRc = False;
                        except:
                            reporter.errorXcpt('Checking status of file #%ld failed:' % (i,));
                            fRc = False;
                            break;
            if fRc:
                reporter.log2('All non-stale files closed');
            cFiles = len(self.oTstDrv.oVBoxMgr.getArray(oGuestSession, 'files'));
            reporter.log2('Final guest session file count: %ld' % (cFiles,));
            # Now try to close the session and see what happens.
            reporter.log2('Closing guest session ...');
            oGuestSession.close();
        except:
            reporter.errorXcpt('Testing for stale processes failed:');
            fRc = False;

        return (fRc, oTxsSession);

    #def testGuestCtrlSessionDirRefs(self, oSession, oTxsSession, oTestVm):
    #    """
    #    Tests the guest session directory reference handling.
    #    """

    #    fRc = True;
    #    return (fRc, oTxsSession);

    def testGuestCtrlSessionProcRefs(self, oSession, oTxsSession, oTestVm): # pylint: disable=R0914
        """
        Tests the guest session process reference handling.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
            sPassword = "password";
            sDomain = "";
            sCmd = "C:\\windows\\system32\\cmd.exe";
            sArgs = [];

        # Number of stale guest processes to create.
        cStaleProcs = 10;

        fRc = True;
        try:
            oGuest = oSession.o.console.guest;
            oGuestSession = oGuest.createSession(sUser, sPassword, sDomain, \
                                                 "testGuestCtrlSessionProcRefs");
            fWaitFor = [ vboxcon.GuestSessionWaitForFlag_Start ];
            waitResult = oGuestSession.waitForArray(fWaitFor, 30 * 1000);
            #
            # Be nice to Guest Additions < 4.3: They don't support session handling and
            # therefore return WaitFlagNotSupported.
            #
            if      waitResult != vboxcon.GuestSessionWaitResult_Start \
                and waitResult != vboxcon.GuestSessionWaitResult_WaitFlagNotSupported:
                # Just log, don't assume an error here (will be done in the main loop then).
                reporter.log('Session did not start successfully, returned wait result: %ld' \
                              % (waitResult));
                return (False, oTxsSession);
            reporter.log('Session successfully started');

            #
            # Fire off forever-running processes and "forget" them (stale entries).
            # For them we don't have any references anymore intentionally.
            #
            reporter.log2('Starting stale processes');
            for i in range(0, cStaleProcs):
                try:
                    oGuestSession.processCreate(sCmd, \
                                                sArgs, [], \
                                                [ vboxcon.ProcessCreateFlag_WaitForStdOut ], \
                                                30 * 1000);
                    # Note: Use a timeout in the call above for not letting the stale processes
                    #       hanging around forever.  This can happen if the installed Guest Additions
                    #       do not support terminating guest processes.
                except:
                    reporter.logXcpt('Creating stale process #%ld failed:' % (i,));
                    fRc = False;
                    break;

            if fRc:
                cProcs = len(self.oTstDrv.oVBoxMgr.getArray(oGuestSession, 'processes'));
                if cProcs != cStaleProcs:
                    reporter.error('Test failed: Got %ld stale processes, expected %ld' % (cProcs, cStaleProcs));
                    fRc = False;

            if fRc:
                #
                # Fire off non-stale processes and wait for termination.
                #
                if oTestVm.isWindows():
                    sArgs = [ '/C', 'dir', '/S', 'C:\\Windows\\system'];
                reporter.log2('Starting non-stale processes');
                aaProcs = [];
                for i in range(0, cStaleProcs):
                    try:
                        oCurProc = oGuestSession.processCreate(sCmd, \
                                                               sArgs, [], \
                                                               [], \
                                                               0); # Infinite timeout.
                        aaProcs.append(oCurProc);
                    except:
                        reporter.logXcpt('Creating non-stale process #%ld failed:' % (i,));
                        fRc = False;
                        break;
            if fRc:
                reporter.log2('Waiting for non-stale processes to terminate');
                for i in range(0, cStaleProcs):
                    try:
                        aaProcs[i].waitForArray([ vboxcon.ProcessWaitForFlag_Terminate ], 30 * 1000);
                        curProcStatus = aaProcs[i].status;
                        if aaProcs[i].status != vboxcon.ProcessStatus_TerminatedNormally:
                            reporter.error('Test failed: Waiting for non-stale processes #%ld'
                                           ' resulted in status %ld, expected %ld' \
                                   % (i, curProcStatus, vboxcon.ProcessStatus_TerminatedNormally));
                            fRc = False;
                    except:
                        reporter.logXcpt('Waiting for non-stale process #%ld failed:' % (i,));
                        fRc = False;
                        break;
                cProcs = len(self.oTstDrv.oVBoxMgr.getArray(oGuestSession, 'processes'));
                # Here we count the stale processes (that is, processes we don't have a reference
                # anymore for) and the started + terminated non-stale processes (that we still keep
                # a reference in aaProcs[] for).
                if cProcs != (cStaleProcs * 2):
                    reporter.error('Test failed: Got %ld total processes, expected %ld' \
                                   % (cProcs, cStaleProcs));
                    fRc = False;
                if fRc:
                    #
                    # Check if all (referenced) non-stale processes now are in "terminated" state.
                    #
                    for i in range(0, cStaleProcs):
                        curProcStatus = aaProcs[i].status;
                        if aaProcs[i].status != vboxcon.ProcessStatus_TerminatedNormally:
                            reporter.error('Test failed: Non-stale processes #%ld has status %ld, expected %ld' \
                                   % (i, curProcStatus, vboxcon.ProcessStatus_TerminatedNormally));
                            fRc = False;
            if fRc:
                reporter.log2('All non-stale processes terminated');

                # Fire off blocking processes which are terminated via terminate().
                if oTestVm.isWindows():
                    sArgs = [ '/C', 'dir', '/S', 'C:\\Windows'];
                reporter.log2('Starting blocking processes');
                aaProcs = [];
                for i in range(0, cStaleProcs):
                    try:
                        oCurProc = oGuestSession.processCreate(sCmd, \
                                                               sArgs, [], \
                                                               [], 30 * 1000);
                        # Note: Use a timeout in the call above for not letting the stale processes
                        #       hanging around forever.  This can happen if the installed Guest Additions
                        #       do not support terminating guest processes.
                        aaProcs.append(oCurProc);
                    except:
                        reporter.logXcpt('Creating blocking process failed:');
                        fRc = False;
                        break;
            if fRc:
                reporter.log2('Terminating blocking processes');
                for i in range(0, cStaleProcs):
                    try:
                        aaProcs[i].terminate();
                    except: # Termination might not be supported, just skip and log it.
                        reporter.logXcpt('Termination of blocking process failed, skipped:');
                cProcs = len(self.oTstDrv.oVBoxMgr.getArray(oGuestSession, 'processes'));
                if cProcs != (cStaleProcs * 2): # Still should be 20 processes because we terminated the 10 newest ones.
                    reporter.error('Test failed: Got %ld total processes, expected %ld' % (cProcs, cStaleProcs * 2));
                    fRc = False;
            cProcs = len(self.oTstDrv.oVBoxMgr.getArray(oGuestSession, 'processes'));
            reporter.log2('Final guest session processes count: %ld' % (cProcs,));
            # Now try to close the session and see what happens.
            reporter.log2('Closing guest session ...');
            oGuestSession.close();
        except:
            reporter.logXcpt('Testing for stale processes failed:');
            fRc = False;

        return (fRc, oTxsSession);

    def testGuestCtrlExec(self, oSession, oTxsSession, oTestVm):                # pylint: disable=R0914,R0915
        """
        Tests the basic execution feature.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
        else:
            sUser = "vbox";
        sPassword = "password";

        if oTestVm.isWindows():
            # Outputting stuff.
            sImageOut = "C:\\windows\\system32\\cmd.exe";
        else:
            reporter.error('Implement me!'); ## @todo Implement non-Windows bits.
            return (False, oTxsSession);

        aaInvalid = [
            # Invalid parameters.
            [ tdTestExec(sUser = sUser, sPassword = sPassword),
              tdTestResultExec(fRc = False) ],
            # Non-existent / invalid image.
            [ tdTestExec(sCmd = "non-existent", sUser = sUser, sPassword = sPassword),
              tdTestResultExec(fRc = False) ],
            [ tdTestExec(sCmd = "non-existent2", sUser = sUser, sPassword = sPassword, fWaitForExit = True),
              tdTestResultExec(fRc = False) ],
            # Use an invalid format string.
            [ tdTestExec(sCmd = "%$%%%&", sUser = sUser, sPassword = sPassword),
              tdTestResultExec(fRc = False) ],
            # More stuff.
            [ tdTestExec(sCmd = "ƒ‰‹ˆ÷‹¸", sUser = sUser, sPassword = sPassword),
              tdTestResultExec(fRc = False) ],
            [ tdTestExec(sCmd = "???://!!!", sUser = sUser, sPassword = sPassword),
              tdTestResultExec(fRc = False) ],
            [ tdTestExec(sCmd = "<>!\\", sUser = sUser, sPassword = sPassword),
              tdTestResultExec(fRc = False) ]
            # Enable as soon as ERROR_BAD_DEVICE is implemented.
            #[ tdTestExec(sCmd = "CON", sUser = sUser, sPassword = sPassword),
            #  tdTestResultExec(fRc = False) ]
        ];

        if oTestVm.isWindows():
            aaExec = [
                # Basic executon.
                [ tdTestExec(sCmd = sImageOut, aArgs = [ '/C', 'dir', '/S', 'c:\\windows\\system32' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True) ],
                [ tdTestExec(sCmd = sImageOut, aArgs = [ '/C', 'dir', '/S', 'c:\\windows\\system32\\kernel32.dll' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True) ],
                [ tdTestExec(sCmd = sImageOut, aArgs = [ '/C', 'dir', '/S', 'c:\\windows\\system32\\nonexist.dll' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True, iExitCode = 1) ],
                [ tdTestExec(sCmd = sImageOut, aArgs = [ '/C', 'dir', '/S', '/wrongparam' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True, iExitCode = 1) ],
                # Paths with spaces.
                ## @todo Get path of installed Guest Additions. Later.
                [ tdTestExec(sCmd = "C:\\Program Files\\Oracle\\VirtualBox Guest Additions\\VBoxControl.exe",
                             aArgs = [ 'version' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True) ],
                # StdOut.
                [ tdTestExec(sCmd = sImageOut, aArgs = [ '/C', 'dir', '/S', 'c:\\windows\\system32' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True) ],
                [ tdTestExec(sCmd = sImageOut, aArgs = [ '/C', 'dir', '/S', 'stdout-non-existing' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True, iExitCode = 1) ],
                # StdErr.
                [ tdTestExec(sCmd = sImageOut, aArgs = [ '/C', 'dir', '/S', 'c:\\windows\\system32' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True) ],
                [ tdTestExec(sCmd = sImageOut, aArgs = [ '/C', 'dir', '/S', 'stderr-non-existing' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True, iExitCode = 1) ],
                # StdOut + StdErr.
                [ tdTestExec(sCmd = sImageOut, aArgs = [ '/C', 'dir', '/S', 'c:\\windows\\system32' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True) ],
                [ tdTestExec(sCmd = sImageOut, aArgs = [ '/C', 'dir', '/S', 'stdouterr-non-existing' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True, iExitCode = 1) ]
                # FIXME: Failing tests.
                # Environment variables.
                # [ tdTestExec(sCmd = sImageOut, aArgs = [ '/C', 'set', 'TEST_NONEXIST' ],
                #              sUser = sUser, sPassword = sPassword),
                #   tdTestResultExec(fRc = True, iExitCode = 1) ]
                # [ tdTestExec(sCmd = sImageOut, aArgs = [ '/C', 'set', 'windir' ],
                #              sUser = sUser, sPassword = sPassword,
                #              aFlags = [ vboxcon.ProcessCreateFlag_WaitForStdOut, vboxcon.ProcessCreateFlag_WaitForStdErr ]),
                #   tdTestResultExec(fRc = True, sBuf = 'windir=C:\\WINDOWS\r\n') ],
                # [ tdTestExec(sCmd = sImageOut, aArgs = [ '/C', 'set', 'TEST_FOO' ],
                #              sUser = sUser, sPassword = sPassword,
                #              aEnv = [ 'TEST_FOO=BAR' ],
                #              aFlags = [ vboxcon.ProcessCreateFlag_WaitForStdOut, vboxcon.ProcessCreateFlag_WaitForStdErr ]),
                #   tdTestResultExec(fRc = True, sBuf = 'TEST_FOO=BAR\r\n') ],
                # [ tdTestExec(sCmd = sImageOut, aArgs = [ '/C', 'set', 'TEST_FOO' ],
                #              sUser = sUser, sPassword = sPassword,
                #              aEnv = [ 'TEST_FOO=BAR', 'TEST_BAZ=BAR' ],
                #              aFlags = [ vboxcon.ProcessCreateFlag_WaitForStdOut, vboxcon.ProcessCreateFlag_WaitForStdErr ]),
                #   tdTestResultExec(fRc = True, sBuf = 'TEST_FOO=BAR\r\n') ]

                ## @todo Create some files (or get files) we know the output size of to validate output length!
                ## @todo Add task which gets killed at some random time while letting the guest output something.
            ];

            # Manual test, not executed automatically.
            aaManual = [
                [ tdTestExec(sCmd = sImageOut, aArgs = [ '/C', 'dir /S C:\\Windows' ],
                             sUser = sUser, sPassword = sPassword,
                             aFlags = [ vboxcon.ProcessCreateFlag_WaitForStdOut, vboxcon.ProcessCreateFlag_WaitForStdErr ]),
                  tdTestResultExec(fRc = True, cbStdOut = 497917) ] ];
        else:
            reporter.log('No OS-specific tests for non-Windows yet!');

        # Build up the final test array for the first batch.
        aaTests = [];
        aaTests.extend(aaInvalid);
        if aaExec is not None:
            aaTests.extend(aaExec);
        fRc = True;

        #
        # Single execution stuff. Nice for debugging.
        #
        fManual = False;
        if fManual:
            curTest = aaTests[1][0]; # tdTestExec, use an index, later.
            curRes  = aaTests[1][1]; # tdTestResultExec
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            fRc, curGuestSession = curTest.createSession('testGuestCtrlExec: Single test 1');
            if fRc is False:
                reporter.error('Single test failed: Could not create session');
            else:
                fRc = self.gctrlExecDoTest(0, curTest, curRes, curGuestSession);
            curTest.closeSession();

            curTest = aaTests[2][0]; # tdTestExec, use an index, later.
            curRes  = aaTests[2][1]; # tdTestResultExec
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            fRc, curGuestSession = curTest.createSession('testGuestCtrlExec: Single test 2');
            if fRc is False:
                reporter.error('Single test failed: Could not create session');
            else:
                fRc = self.gctrlExecDoTest(0, curTest, curRes, curGuestSession);
            curTest.closeSession();

            curTest = aaTests[3][0]; # tdTestExec, use an index, later.
            curRes  = aaTests[3][1]; # tdTestResultExec
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            fRc, curGuestSession = curTest.createSession('testGuestCtrlExec: Single test 3');
            if fRc is False:
                reporter.error('Single test failed: Could not create session');
            else:
                fRc = self.gctrlExecDoTest(0, curTest, curRes, curGuestSession);
            curTest.closeSession();
            return (fRc, oTxsSession);
        else:
            aaManual = aaManual; # Workaround for pylint #W0612.

        if fRc is False:
            return (fRc, oTxsSession);

        #
        # First batch: One session per guest process.
        #
        for (i, aTest) in enumerate(aaTests):
            curTest = aTest[0]; # tdTestExec, use an index, later.
            curRes  = aTest[1]; # tdTestResultExec
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            fRc, curGuestSession = curTest.createSession('testGuestCtrlExec: Test #%d' % (i,));
            if fRc is False:
                reporter.error('Test #%d failed: Could not create session' % (i,));
                break;
            fRc = self.gctrlExecDoTest(i, curTest, curRes, curGuestSession);
            if fRc is False:
                break;
            fRc = curTest.closeSession();
            if fRc is False:
                break;

        # No sessions left?
        if fRc is True:
            cSessions = len(self.oTstDrv.oVBoxMgr.getArray(oSession.o.console.guest, 'sessions'));
            if cSessions is not 0:
                reporter.error('Found %d stale session(s), expected 0' % (cSessions,));
                fRc = False;

        if fRc is False:
            return (fRc, oTxsSession);

        reporter.log('Now using one guest session for all tests ...');

        #
        # Second batch: One session for *all* guest processes.
        #
        oGuest = oSession.o.console.guest;
        try:
            reporter.log('Creating session for all tests ...');
            curGuestSession = oGuest.createSession(sUser, sPassword, '', 'testGuestCtrlExec: One session for all tests');
            try:
                fWaitFor = [ vboxcon.GuestSessionWaitForFlag_Start ];
                waitResult = curGuestSession.waitForArray(fWaitFor, 30 * 1000);
                if      waitResult != vboxcon.GuestSessionWaitResult_Start \
                    and waitResult != vboxcon.GuestSessionWaitResult_WaitFlagNotSupported:
                    reporter.error('Session did not start successfully, returned wait result: %ld' \
                                   % (waitResult));
                    return (False, oTxsSession);
                reporter.log('Session successfully started');
            except:
                # Just log, don't assume an error here (will be done in the main loop then).
                reporter.logXcpt('Waiting for guest session to start failed:');
                return (False, oTxsSession);
            # Note: Not waiting for the guest session to start here
            #       is intentional. This must be handled by the process execution
            #       call then.
            for (i, aTest) in enumerate(aaTests):
                curTest = aTest[0]; # tdTestExec, use an index, later.
                curRes  = aTest[1]; # tdTestResultExec
                curTest.setEnvironment(oSession, oTxsSession, oTestVm);
                fRc = self.gctrlExecDoTest(i, curTest, curRes, curGuestSession);
                if fRc is False:
                    break;
            try:
                reporter.log2('Closing guest session ...');
                curGuestSession.close();
                curGuestSession = None;
            except:
                # Just log, don't assume an error here (will be done in the main loop then).
                reporter.logXcpt('Closing guest session failed:');
                fRc = False;
        except:
            reporter.logXcpt('Could not create one session:');

        # No sessions left?
        if fRc is True:
            cSessions = len(self.oTstDrv.oVBoxMgr.getArray(oSession.o.console.guest, 'sessions'));
            if cSessions is not 0:
                reporter.error('Found %d stale session(s), expected 0' % (cSessions,));
                fRc = False;

        return (fRc, oTxsSession);

    def testGuestCtrlExecErrorLevel(self, oSession, oTxsSession, oTestVm):
        """
        Tests handling of error levels from started guest processes.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
        else:
            sUser = "vbox";
        sPassword = "password";

        if oTestVm.isWindows():
            # Outputting stuff.
            sImage = "C:\\windows\\system32\\cmd.exe";
        else:
            reporter.error('Implement me!'); ## @todo Implement non-Windows bits.
            return (False, oTxsSession);

        aaTests = [];
        if oTestVm.isWindows():
            aaTests.extend([
                # Simple.
                [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'wrongcommand' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True, iExitCode = 1) ],
                [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'exit', '22' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True, iExitCode = 22) ],
                [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'set', 'ERRORLEVEL=234' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True, iExitCode = 0) ],
                [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'echo', '%WINDIR%' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True, iExitCode = 0) ],
                [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'set', 'ERRORLEVEL=0' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True, iExitCode = 0) ],
                [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'dir', 'c:\\windows\\system32' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True, iExitCode = 0) ],
                [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'dir', 'c:\\windows\\system32\\kernel32.dll' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True, iExitCode = 0) ],
                [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'dir', 'c:\\nonexisting-file' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True, iExitCode = 1) ],
                [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'dir', 'c:\\nonexisting-dir\\' ],
                             sUser = sUser, sPassword = sPassword),
                  tdTestResultExec(fRc = True, iExitCode = 1) ]
                # FIXME: Failing tests.
                # With stdout.
                # [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'dir', 'c:\\windows\\system32' ],
                #              sUser = sUser, sPassword = sPassword, aFlags = [ vboxcon.ProcessCreateFlag_WaitForStdOut ]),
                #   tdTestResultExec(fRc = True, iExitCode = 0) ],
                # [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'dir', 'c:\\nonexisting-file' ],
                #              sUser = sUser, sPassword = sPassword, aFlags = [ vboxcon.ProcessCreateFlag_WaitForStdOut ]),
                #   tdTestResultExec(fRc = True, iExitCode = 1) ],
                # [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'dir', 'c:\\nonexisting-dir\\' ],
                #              sUser = sUser, sPassword = sPassword, aFlags = [ vboxcon.ProcessCreateFlag_WaitForStdOut ]),
                #   tdTestResultExec(fRc = True, iExitCode = 1) ],
                # With stderr.
                # [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'dir', 'c:\\windows\\system32' ],
                #              sUser = sUser, sPassword = sPassword, aFlags = [ vboxcon.ProcessCreateFlag_WaitForStdErr ]),
                #   tdTestResultExec(fRc = True, iExitCode = 0) ],
                # [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'dir', 'c:\\nonexisting-file' ],
                #              sUser = sUser, sPassword = sPassword, aFlags = [ vboxcon.ProcessCreateFlag_WaitForStdErr ]),
                #   tdTestResultExec(fRc = True, iExitCode = 1) ],
                # [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'dir', 'c:\\nonexisting-dir\\' ],
                #              sUser = sUser, sPassword = sPassword, aFlags = [ vboxcon.ProcessCreateFlag_WaitForStdErr ]),
                #   tdTestResultExec(fRc = True, iExitCode = 1) ],
                # With stdout/stderr.
                # [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'dir', 'c:\\windows\\system32' ],
                #              sUser = sUser, sPassword = sPassword,
                #              aFlags = [ vboxcon.ProcessCreateFlag_WaitForStdOut, vboxcon.ProcessCreateFlag_WaitForStdErr ]),
                #   tdTestResultExec(fRc = True, iExitCode = 0) ],
                # [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'dir', 'c:\\nonexisting-file' ],
                #              sUser = sUser, sPassword = sPassword,
                #              aFlags = [ vboxcon.ProcessCreateFlag_WaitForStdOut, vboxcon.ProcessCreateFlag_WaitForStdErr ]),
                #   tdTestResultExec(fRc = True, iExitCode = 1) ],
                # [ tdTestExec(sCmd = sImage, aArgs = [ '/C', 'dir', 'c:\\nonexisting-dir\\' ],
                #              sUser = sUser, sPassword = sPassword,
                #              aFlags = [ vboxcon.ProcessCreateFlag_WaitForStdOut, vboxcon.ProcessCreateFlag_WaitForStdErr ]),
                #   tdTestResultExec(fRc = True, iExitCode = 1) ]
                ## @todo Test stdin!
            ]);
        else:
            reporter.log('No OS-specific tests for non-Windows yet!');

        fRc = True;
        for (i, aTest) in enumerate(aaTests):
            curTest = aTest[0]; # tdTestExec, use an index, later.
            curRes  = aTest[1]; # tdTestResult
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            fRc, curGuestSession = curTest.createSession('testGuestCtrlExecErrorLevel: Test #%d' % (i,));
            if fRc is False:
                reporter.error('Test #%d failed: Could not create session' % (i,));
                break;
            fRc = self.gctrlExecDoTest(i, curTest, curRes, curGuestSession);
            curTest.closeSession();
            if fRc is False:
                break;

        return (fRc, oTxsSession);

    def testGuestCtrlExecTimeout(self, oSession, oTxsSession, oTestVm):
        """
        Tests handling of timeouts of started guest processes.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
        else:
            sUser = "vbox";
        sPassword = "password";
        sDomain = "";

        if oTestVm.isWindows():
            # Outputting stuff.
            sImage = "C:\\windows\\system32\\cmd.exe";
        else:
            reporter.error('Implement me!'); ## @todo Implement non-Windows bits.
            return (False, oTxsSession);

        fRc = True;
        try:
            oGuest = oSession.o.console.guest;
            oGuestSession = oGuest.createSession(sUser, sPassword, sDomain, "testGuestCtrlExecTimeout");
            oGuestSession.waitForArray([ vboxcon.GuestSessionWaitForFlag_Start ], 30 * 1000);
            # Create a process which never terminates and should timeout when
            # waiting for termination.
            try:
                curProc = oGuestSession.processCreate(sImage, [], \
                                                      [], [], 30 * 1000);
                reporter.log('Waiting for process 1 being started ...');
                waitRes = curProc.waitForArray([ vboxcon.ProcessWaitForFlag_Start ], 30 * 1000);
                if waitRes != vboxcon.ProcessWaitResult_Start:
                    reporter.error('Waiting for process 1 to start failed, got status %ld');
                    fRc = False;
                if fRc:
                    reporter.log('Waiting for process 1 to time out within 1ms ...');
                    waitRes = curProc.waitForArray([ vboxcon.ProcessWaitForFlag_Terminate ], 1);
                    if waitRes != vboxcon.ProcessWaitResult_Timeout:
                        reporter.error('Waiting for process 1 did not time out when it should (1)');
                        fRc = False;
                    else:
                        reporter.log('Waiting for process 1 timed out (1), good');
                if fRc:
                    reporter.log('Waiting for process 1 to time out within 5000ms ...');
                    waitRes = curProc.waitForArray([ vboxcon.ProcessWaitForFlag_Terminate ], 5000);
                    if waitRes != vboxcon.ProcessWaitResult_Timeout:
                        reporter.error('Waiting for process 1 did not time out when it should, got wait result %ld' % (waitRes,));
                        fRc = False;
                    else:
                        reporter.log('Waiting for process 1 timed out (5000), good');
                ## @todo Add curProc.terminate() as soon as it's implemented.
            except:
                reporter.errorXcpt('Exception for process 1:');
                fRc = False;
            # Create a lengthly running guest process which will be killed by VBoxService on the
            # guest because it ran out of execution time (5 seconds).
            if fRc:
                try:
                    curProc = oGuestSession.processCreate(sImage, [], \
                                                          [], [], 5 * 1000);
                    reporter.log('Waiting for process 2 being started ...');
                    waitRes = curProc.waitForArray([ vboxcon.ProcessWaitForFlag_Start ], 30 * 1000);
                    if waitRes != vboxcon.ProcessWaitResult_Start:
                        reporter.error('Waiting for process 1 to start failed, got status %ld');
                        fRc = False;
                    if fRc:
                        reporter.log('Waiting for process 2 to get killed because it ran out of execution time ...');
                        waitRes = curProc.waitForArray([ vboxcon.ProcessWaitForFlag_Terminate ], 30 * 1000);
                        if waitRes != vboxcon.ProcessWaitResult_Timeout:
                            reporter.error('Waiting for process 2 did not time out when it should, got wait result %ld' \
                                           % (waitRes,));
                            fRc = False;
                    if fRc:
                        reporter.log('Waiting for process 2 indicated an error, good');
                        if curProc.status != vboxcon.ProcessStatus_TimedOutKilled:
                            reporter.error('Status of process 2 wrong; excepted %ld, got %ld' \
                                           % (vboxcon.ProcessStatus_TimedOutKilled, curProc.status));
                            fRc = False;
                        else:
                            reporter.log('Status of process 2 correct (%ld)' % (vboxcon.ProcessStatus_TimedOutKilled,));
                    ## @todo Add curProc.terminate() as soon as it's implemented.
                except:
                    reporter.errorXcpt('Exception for process 2:');
                    fRc = False;
            oGuestSession.close();
        except:
            reporter.errorXcpt('Could not handle session:');
            fRc = False;

        return (fRc, oTxsSession);

    def testGuestCtrlDirCreate(self, oSession, oTxsSession, oTestVm):
        """
        Tests creation of guest directories.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
        else:
            sUser = "vbox";
        sPassword = "password";

        if oTestVm.isWindows():
            sScratch  = "C:\\Temp\\vboxtest\\testGuestCtrlDirCreate\\";

        aaTests = [];
        if oTestVm.isWindows():
            aaTests.extend([
                # Invalid stuff.
                [ tdTestDirCreate(sUser = sUser, sPassword = sPassword, sDirectory = '' ),
                  tdTestResult(fRc = False) ],
                # More unusual stuff.
                [ tdTestDirCreate(sUser = sUser, sPassword = sPassword, sDirectory = '..\\..\\' ),
                  tdTestResult(fRc = False) ],
                [ tdTestDirCreate(sUser = sUser, sPassword = sPassword, sDirectory = '../../' ),
                  tdTestResult(fRc = False) ],
                [ tdTestDirCreate(sUser = sUser, sPassword = sPassword, sDirectory = 'z:\\' ),
                  tdTestResult(fRc = False) ],
                [ tdTestDirCreate(sUser = sUser, sPassword = sPassword, sDirectory = '\\\\uncrulez\\foo' ),
                  tdTestResult(fRc = False) ],
                # Creating directories.
                [ tdTestDirCreate(sUser = sUser, sPassword = sPassword, sDirectory = sScratch ),
                  tdTestResult(fRc = False) ],
                [ tdTestDirCreate(sUser = sUser, sPassword = sPassword, sDirectory = os.path.join(sScratch, 'foo\\bar\\baz'),
                                  aFlags = [ vboxcon.DirectoryCreateFlag_Parents ] ),
                  tdTestResult(fRc = True) ],
                [ tdTestDirCreate(sUser = sUser, sPassword = sPassword, sDirectory = os.path.join(sScratch, 'foo\\bar\\baz'),
                                  aFlags = [ vboxcon.DirectoryCreateFlag_Parents ] ),
                  tdTestResult(fRc = True) ],
                # Long (+ random) stuff.
                [ tdTestDirCreate(sUser = sUser, sPassword = sPassword,
                                  sDirectory = os.path.join(sScratch,
                                  "".join(random.choice(string.ascii_lowercase) for i in range(32))) ),
                  tdTestResult(fRc = True) ],
                [ tdTestDirCreate(sUser = sUser, sPassword = sPassword,
                                  sDirectory = os.path.join(sScratch,
                                  "".join(random.choice(string.ascii_lowercase) for i in range(128))) ),
                  tdTestResult(fRc = True) ],
                # Following two should fail on Windows (paths too long). Both should timeout.
                [ tdTestDirCreate(sUser = sUser, sPassword = sPassword,
                                  sDirectory = os.path.join(sScratch,
                                  "".join(random.choice(string.ascii_lowercase) for i in range(255))) ),
                  tdTestResult(fRc = False) ],
                [ tdTestDirCreate(sUser = sUser, sPassword = sPassword,
                                  sDirectory = os.path.join(sScratch,
                                  "".join(random.choice(string.ascii_lowercase) for i in range(1024))) ),
                  tdTestResult(fRc = False) ]
            ]);
        else:
            reporter.log('No OS-specific tests for non-Windows yet!');

        fRc = True;
        for (i, aTest) in enumerate(aaTests):
            curTest = aTest[0]; # tdTestExec, use an index, later.
            curRes  = aTest[1]; # tdTestResult
            reporter.log('Testing #%d, sDirectory="%s" ...' % (i, curTest.sDirectory));
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            fRc, curGuestSession = curTest.createSession('testGuestCtrlDirCreate: Test #%d' % (i,));
            if fRc is False:
                reporter.error('Test #%d failed: Could not create session' % (i,));
                break;
            fRc = self.gctrlCreateDir(curTest, curRes, curGuestSession);
            curTest.closeSession();
            if fRc is False:
                reporter.error('Test #%d failed' % (i,));
                fRc = False;
                break;

        return (fRc, oTxsSession);

    def testGuestCtrlDirCreateTemp(self, oSession, oTxsSession, oTestVm): # pylint: disable=R0914
        """
        Tests creation of temporary directories.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
        else:
            sUser = "vbox";
        sPassword = "password";

        # if oTestVm.isWindows():
        #     sScratch = "C:\\Temp\\vboxtest\\testGuestCtrlDirCreateTemp\\";

        aaTests = [];
        if oTestVm.isWindows():
            aaTests.extend([
                # Invalid stuff.
                [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sDirectory = ''),
                  tdTestResult(fRc = False) ],
                [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sDirectory = 'C:\\Windows',
                                      fMode = 1234),
                  tdTestResult(fRc = False) ],
                [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = '',
                                      sDirectory = 'C:\\Windows', fMode = 1234),
                  tdTestResult(fRc = False) ],
                [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'xXx',
                                      sDirectory = 'C:\\Windows', fMode = 0700),
                  tdTestResult(fRc = False) ],
                [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'xxx',
                                      sDirectory = 'C:\\Windows', fMode = 0700),
                  tdTestResult(fRc = False) ],
                # More unusual stuff.
                [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'foo',
                                      sDirectory = 'z:\\'),
                  tdTestResult(fRc = False) ],
                [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'foo',
                                      sDirectory = '\\\\uncrulez\\foo'),
                  tdTestResult(fRc = False) ],
                # Non-existing stuff.
                [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'bar',
                                      sDirectory = 'c:\\Apps\\nonexisting\\foo'),
                  tdTestResult(fRc = False) ],
                # FIXME: Failing test. Non Windows path
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'bar',
                #                       sDirectory = '/tmp/non/existing'),
                #   tdTestResult(fRc = False) ]
            ]);
        else:
            reporter.log('No OS-specific tests for non-Windows yet!');

            # FIXME: Failing tests.
            # aaTests.extend([
                # Non-secure variants.
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'XXX',
                #                       sDirectory = sScratch),
                #   tdTestResult(fRc = True) ],
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'XXX',
                #                       sDirectory = sScratch),
                #   tdTestResult(fRc = True) ],
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'X',
                #                       sDirectory = sScratch),
                #   tdTestResult(fRc = True) ],
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'X',
                #                       sDirectory = sScratch),
                #   tdTestResult(fRc = True) ],
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'XXX',
                #                       sDirectory = sScratch,
                #                       fMode = 0700),
                #   tdTestResult(fRc = True) ],
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'XXX',
                #                     sDirectory = sScratch,
                #                     fMode = 0700),
                #   tdTestResult(fRc = True) ],
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'XXX',
                #                       sDirectory = sScratch,
                #                       fMode = 0755),
                #   tdTestResult(fRc = True) ],
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'XXX',
                #                       sDirectory = sScratch,
                #                       fMode = 0755),
                #   tdTestResult(fRc = True) ],
                # Secure variants.
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'XXX',
                #                       sDirectory = sScratch, fSecure = True),
                #   tdTestResult(fRc = True) ],
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'XXX',
                #                       sDirectory = sScratch, fSecure = True),
                #   tdTestResult(fRc = True) ],
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'XXX',
                #                       sDirectory = sScratch, fSecure = True),
                #   tdTestResult(fRc = True) ],
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'XXX',
                #                       sDirectory = sScratch, fSecure = True),
                #   tdTestResult(fRc = True) ],
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'XXX',
                #                       sDirectory = sScratch,
                #                       fSecure = True, fMode = 0700),
                #   tdTestResult(fRc = True) ],
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'XXX',
                #                       sDirectory = sScratch,
                #                       fSecure = True, fMode = 0700),
                #   tdTestResult(fRc = True) ],
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'XXX',
                #                       sDirectory = sScratch,
                #                       fSecure = True, fMode = 0755),
                #   tdTestResult(fRc = True) ],
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = 'XXX',
                #                       sDirectory = sScratch,
                #                       fSecure = True, fMode = 0755),
                #   tdTestResult(fRc = True) ],
                # Random stuff.
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword,
                #                       sTemplate = "XXX-".join(random.choice(string.ascii_lowercase) for i in range(32)),
                #                       sDirectory = sScratch,
                #                       fSecure = True, fMode = 0755),
                #   tdTestResult(fRc = True) ],
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = "".join('X' for i in range(32)),
                #                       sDirectory = sScratch,
                #                       fSecure = True, fMode = 0755),
                #   tdTestResult(fRc = True) ],
                # [ tdTestDirCreateTemp(sUser = sUser, sPassword = sPassword, sTemplate = "".join('X' for i in range(128)),
                #                       sDirectory = sScratch,
                #                       fSecure = True, fMode = 0755),
                #   tdTestResult(fRc = True) ]
            # ]);

        fRc = True;
        for (i, aTest) in enumerate(aaTests):
            curTest = aTest[0]; # tdTestExec, use an index, later.
            curRes  = aTest[1]; # tdTestResult
            reporter.log('Testing #%d, sTemplate="%s", fMode=%#o, path="%s", secure="%s" ...' %
                         (i, curTest.sTemplate, curTest.fMode, curTest.sDirectory, curTest.fSecure));
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            fRc, curGuestSession = curTest.createSession('testGuestCtrlDirCreateTemp: Test #%d' % (i,));
            if fRc is False:
                reporter.error('Test #%d failed: Could not create session' % (i,));
                break;
            sDirTemp = "";
            try:
                sDirTemp = curGuestSession.directoryCreateTemp(curTest.sTemplate, curTest.fMode,
                                                               curTest.sDirectory, curTest.fSecure);
            except:
                if curRes.fRc is True:
                    reporter.errorXcpt('Creating temp directory "%s" failed:' % (curTest.sDirectory,));
                    fRc = False;
                    break;
                else:
                    reporter.logXcpt('Creating temp directory "%s" failed expectedly, skipping:' % (curTest.sDirectory,));
            curTest.closeSession();
            if sDirTemp  != "":
                reporter.log2('Temporary directory is: %s' % (sDirTemp,));
                fExists = curGuestSession.directoryExists(sDirTemp);
                if fExists is False:
                    reporter.error('Test #%d failed: Temporary directory "%s" does not exists' % (i, sDirTemp));
                    fRc = False;
                    break;
        return (fRc, oTxsSession);

    def testGuestCtrlDirRead(self, oSession, oTxsSession, oTestVm): # pylint: disable=R0914
        """
        Tests opening and reading (enumerating) guest directories.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
        else:
            sUser = "vbox";
        sPassword = "password";

        aaTests = [];
        if oTestVm.isWindows():
            aaTests.extend([
                # Invalid stuff.
                [ tdTestDirRead(sUser = sUser, sPassword = sPassword, sDirectory = ''),
                  tdTestResultDirRead(fRc = False) ],
                [ tdTestDirRead(sUser = sUser, sPassword = sPassword, sDirectory = 'C:\\Windows', aFlags = [ 1234 ]),
                  tdTestResultDirRead(fRc = False) ],
                [ tdTestDirRead(sUser = sUser, sPassword = sPassword, sDirectory = 'C:\\Windows', sFilter = '*.foo'),
                  tdTestResultDirRead(fRc = False) ],
                # More unusual stuff.
                [ tdTestDirRead(sUser = sUser, sPassword = sPassword, sDirectory = 'z:\\'),
                  tdTestResultDirRead(fRc = False) ],
                [ tdTestDirRead(sUser = sUser, sPassword = sPassword, sDirectory = '\\\\uncrulez\\foo'),
                  tdTestResultDirRead(fRc = False) ],
                # Non-existing stuff.
                [ tdTestDirRead(sUser = sUser, sPassword = sPassword, sDirectory = 'c:\\Apps\\nonexisting'),
                  tdTestResultDirRead(fRc = False) ],
                [ tdTestDirRead(sUser = sUser, sPassword = sPassword, sDirectory = 'c:\\Apps\\testDirRead'),
                  tdTestResultDirRead(fRc = False) ]
            ]);

            if oTestVm.sVmName == 'tst-xppro':
                aaTests.extend([
                    # Reading directories.
                    [ tdTestDirRead(sUser = sUser, sPassword = sPassword, sDirectory = '../../Windows/Fonts'),
                      tdTestResultDirRead(fRc = True, numFiles = 191) ],
                    [ tdTestDirRead(sUser = sUser, sPassword = sPassword, sDirectory = 'c:\\Windows\\Help'),
                      tdTestResultDirRead(fRc = True, numDirs = 13, numFiles = 569) ],
                    [ tdTestDirRead(sUser = sUser, sPassword = sPassword, sDirectory = 'c:\\Windows\\Web'),
                      tdTestResultDirRead(fRc = True, numDirs = 3, numFiles = 55) ]
                ]);
        else:
            reporter.log('No OS-specific tests for non-Windows yet!');

        fRc = True;
        for (i, aTest) in enumerate(aaTests):
            curTest = aTest[0]; # tdTestExec, use an index, later.
            curRes  = aTest[1]; # tdTestResult
            reporter.log('Testing #%d, dir="%s" ...' % (i, curTest.sDirectory));
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            fRc, curGuestSession = curTest.createSession('testGuestCtrlDirRead: Test #%d' % (i,));
            if fRc is False:
                reporter.error('Test #%d failed: Could not create session' % (i,));
                break;
            (fRc2, cDirs, cFiles) = self.gctrlReadDir(curTest, curRes, curGuestSession);
            curTest.closeSession();
            reporter.log2('Test #%d: Returned %ld directories, %ld files total' % (i, cDirs, cFiles));
            if fRc2 is curRes.fRc:
                if fRc2 is True:
                    if curRes.numFiles != cFiles:
                        reporter.error('Test #%d failed: Got %ld files, expected %ld' % (i, cFiles, curRes.numFiles));
                        fRc = False;
                        break;
                    if curRes.numDirs != cDirs:
                        reporter.error('Test #%d failed: Got %ld directories, expected %ld' % (i, cDirs, curRes.numDirs));
                        fRc = False;
                        break;
            else:
                reporter.error('Test #%d failed: Got %s, expected %s' % (i, fRc2, curRes.fRc));
                fRc = False;
                break;

        return (fRc, oTxsSession);

    def testGuestCtrlFileRemove(self, oSession, oTxsSession, oTestVm):
        """
        Tests removing guest files.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
        else:
            sUser = "vbox";
        sPassword = "password";

        aaTests = [];
        if oTestVm.isWindows():
            aaTests.extend([
                # Invalid stuff.
                [ tdTestFileRemove(sUser = sUser, sPassword = sPassword, sFile = ''),               tdTestResult(fRc = False) ],
                [ tdTestFileRemove(sUser = sUser, sPassword = sPassword, sFile = 'C:\\Windows'),    tdTestResult(fRc = False) ],
                [ tdTestFileRemove(sUser = sUser, sPassword = sPassword, sFile = 'C:\\Windows'),    tdTestResult(fRc = False) ],
                # More unusual stuff.
                [ tdTestFileRemove(sUser = sUser, sPassword = sPassword, sFile = 'z:\\'),           tdTestResult(fRc = False) ],
                [ tdTestFileRemove(sUser = sUser, sPassword = sPassword, sFile = '\\\\uncrulez\\foo'),
                                                                                                   tdTestResult(fRc = False) ],
                # Non-existing stuff.
                [ tdTestFileRemove(sUser = sUser, sPassword = sPassword, sFile = 'c:\\Apps\\nonexisting'),
                                                                                                   tdTestResult(fRc = False) ],
                [ tdTestFileRemove(sUser = sUser, sPassword = sPassword, sFile = 'c:\\Apps\\testFileRemove'),
                                                                                                   tdTestResult(fRc = False) ],
                # Try to delete system files.
                [ tdTestFileRemove(sUser = sUser, sPassword = sPassword, sFile = 'c:\\pagefile.sys'),
                                                                                                   tdTestResult(fRc = False) ],
                [ tdTestFileRemove(sUser = sUser, sPassword = sPassword, sFile = 'c:\\Windows\\kernel32.sys'),
                                                                                                   tdTestResult(fRc = False) ]
            ]);

            if oTestVm.sVmName == 'tst-xppro':
                aaTests.extend([
                    # Try delete some unimportant media stuff.
                    [ tdTestFileRemove(sUser = sUser, sPassword = sPassword, sFile = 'c:\\Windows\\Media\\chimes.wav'),
                                                                                                   tdTestResult(fRc = True) ],
                    # Second attempt should fail.
                    [ tdTestFileRemove(sUser = sUser, sPassword = sPassword, sFile = 'c:\\Windows\\Media\\chimes.wav'),
                                                                                                   tdTestResult(fRc = False) ],
                    [ tdTestFileRemove(sUser = sUser, sPassword = sPassword, sFile = 'c:\\Windows\\Media\\chord.wav'),
                                                                                                   tdTestResult(fRc = True) ],
                    [ tdTestFileRemove(sUser = sUser, sPassword = sPassword, sFile = 'c:\\Windows\\Media\\chord.wav'),
                                                                                                   tdTestResult(fRc = False) ]
                ]);
        else:
            reporter.log('No OS-specific tests for non-Windows yet!');

        fRc = True;
        for (i, aTest) in enumerate(aaTests):
            curTest = aTest[0]; # tdTestExec, use an index, later.
            curRes  = aTest[1]; # tdTestResult
            reporter.log('Testing #%d, file="%s" ...' % (i, curTest.sFile));
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            fRc, curGuestSession = curTest.createSession('testGuestCtrlFileRemove: Test #%d' % (i,));
            if fRc is False:
                reporter.error('Test #%d failed: Could not create session' % (i,));
                break;
            try:
                curGuestSession.fileRemove(curTest.sFile);
            except:
                if curRes.fRc is True:
                    reporter.errorXcpt('Removing file "%s" failed:' % (curTest.sFile,));
                    fRc = False;
                    break;
                else:
                    reporter.logXcpt('Removing file "%s" failed expectedly, skipping:' % (curTest.sFile,));
            curTest.closeSession();
        return (fRc, oTxsSession);

    def testGuestCtrlFileStat(self, oSession, oTxsSession, oTestVm): # pylint: disable=R0914
        """
        Tests querying file information through stat.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
        else:
            sUser = "vbox";
        sPassword = "password";

        aaTests = [];
        if oTestVm.isWindows():
            aaTests.extend([
                # Invalid stuff.
                [ tdTestFileStat(sUser = sUser, sPassword = sPassword, sFile = ''),
                  tdTestResultFileStat(fRc = False) ],
                [ tdTestFileStat(sUser = sUser, sPassword = sPassword, sFile = 'C:\\Windows'),
                  tdTestResultFileStat(fRc = False) ],
                [ tdTestFileStat(sUser = sUser, sPassword = sPassword, sFile = 'C:\\Windows'),
                  tdTestResultFileStat(fRc = False) ],
                # More unusual stuff.
                [ tdTestFileStat(sUser = sUser, sPassword = sPassword, sFile = 'z:\\'),
                  tdTestResultFileStat(fRc = False) ],
                [ tdTestFileStat(sUser = sUser, sPassword = sPassword, sFile = '\\\\uncrulez\\foo'),
                  tdTestResultFileStat(fRc = False) ],
                # Non-existing stuff.
                [ tdTestFileStat(sUser = sUser, sPassword = sPassword, sFile = 'c:\\Apps\\nonexisting'),
                  tdTestResultFileStat(fRc = False) ],
                [ tdTestFileStat(sUser = sUser, sPassword = sPassword, sFile = 'c:\\Apps\\testDirRead'),
                  tdTestResultFileStat(fRc = False) ]
            ]);

            if oTestVm.sVmName == 'tst-xppro':
                aaTests.extend([
                    # Directories; should fail.
                    [ tdTestFileStat(sUser = sUser, sPassword = sPassword, sFile = '../../Windows/Fonts'),
                      tdTestResultFileStat(fRc = False) ],
                    [ tdTestFileStat(sUser = sUser, sPassword = sPassword, sFile = 'c:\\Windows\\Help'),
                      tdTestResultFileStat(fRc = False) ],
                    [ tdTestFileStat(sUser = sUser, sPassword = sPassword, sFile = 'c:\\Windows\\system32'),
                      tdTestResultFileStat(fRc = False) ],
                    # Regular files.
                    [ tdTestFileStat(sUser = sUser, sPassword = sPassword, sFile = 'c:\\Windows\\system32\\kernel32.dll'),
                      tdTestResultFileStat(fRc = False, cbSize = 926720, eFileType = vboxcon.FsObjType_File) ]
                ]);
        else:
            reporter.log('No OS-specific tests for non-Windows yet!');

        fRc = True;
        for (i, aTest) in enumerate(aaTests):
            curTest = aTest[0]; # tdTestExec, use an index, later.
            curRes  = aTest[1]; # tdTestResult
            reporter.log('Testing #%d, sFile="%s" ...' % (i, curTest.sFile));
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            fRc, curGuestSession = curTest.createSession('testGuestCtrlFileStat: Test #%d' % (i,));
            if fRc is False:
                reporter.error('Test #%d failed: Could not create session' % (i,));
                break;
            fileObjInfo = None;
            try:
                fileObjInfo = curGuestSession.fileQueryInfo(curTest.sFile);
            except:
                if curRes.fRc is True:
                    reporter.errorXcpt('Querying file information for "%s" failed:' % (curTest.sFile,));
                    fRc = False;
                    break;
                else:
                    reporter.logXcpt('Querying file information for "%s" failed expectedly, skipping:' % (curTest.sFile,));
            curTest.closeSession();
            if fileObjInfo is not None:
                eFileType = fileObjInfo.type;
                if curRes.eFileType != eFileType:
                    reporter.error('Test #%d failed: Got file type %ld, expected %ld' % (i, eFileType, curRes.eFileType));
                    fRc = False;
                    break;
                cbFile = long(fileObjInfo.objectSize);
                if curRes.cbSize != cbFile:
                    reporter.error('Test #%d failed: Got %ld bytes size, expected %ld bytes' % (i, cbFile, curRes.cbSize));
                    fRc = False;
                    break;
                ## @todo Add more checks later.

        return (fRc, oTxsSession);

    def testGuestCtrlFileRead(self, oSession, oTxsSession, oTestVm): # pylint: disable=R0914
        """
        Tests reading from guest files.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
        else:
            sUser = "vbox";
        sPassword = "password";

        if oTxsSession.syncMkDir('${SCRATCH}/testGuestCtrlFileRead') is False:
            reporter.error('Could not create scratch directory on guest');
            return (False, oTxsSession);

        aaTests = [];
        aaTests.extend([
            # Invalid stuff.
            [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, cbToReadWrite = 0),
              tdTestResultFileReadWrite(fRc = False) ],
            [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = ''),
              tdTestResultFileReadWrite(fRc = False) ],
            [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = 'non-existing.file'),
              tdTestResultFileReadWrite(fRc = False) ],
            # Wrong open mode.
            [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = 'non-existing.file', \
                                  sOpenMode = 'rt', sDisposition = 'oe'),
              tdTestResultFileReadWrite(fRc = False) ],
            [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = '\\\\uncrulez\\non-existing.file', \
                                  sOpenMode = 'tr', sDisposition = 'oe'),
              tdTestResultFileReadWrite(fRc = False) ],
            [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = '../../non-existing.file', \
                                  sOpenMode = 'wr', sDisposition = 'oe'),
              tdTestResultFileReadWrite(fRc = False) ],
            # Wrong disposition.
            [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = 'non-existing.file', \
                                  sOpenMode = 'r', sDisposition = 'e'),
              tdTestResultFileReadWrite(fRc = False) ],
            [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = '\\\\uncrulez\\non-existing.file', \
                                  sOpenMode = 'r', sDisposition = 'o'),
              tdTestResultFileReadWrite(fRc = False) ],
            [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = '../../non-existing.file', \
                                  sOpenMode = 'r', sDisposition = 'c'),
              tdTestResultFileReadWrite(fRc = False) ],
            # Opening non-existing file when it should exist.
            [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = 'non-existing.file', \
                                  sOpenMode = 'r', sDisposition = 'oe'),
              tdTestResultFileReadWrite(fRc = False) ],
            [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = '\\\\uncrulez\\non-existing.file', \
                                  sOpenMode = 'r', sDisposition = 'oe'),
              tdTestResultFileReadWrite(fRc = False) ],
            [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = '../../non-existing.file', \
                                  sOpenMode = 'r', sDisposition = 'oe'),
              tdTestResultFileReadWrite(fRc = False) ]
        ]);

        if oTestVm.isWindows():
            aaTests.extend([
                # Create a file which must not exist (but it hopefully does).
                [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = 'C:\\Windows\\System32\\calc.exe', \
                                      sOpenMode = 'w', sDisposition = 'ce'),
                  tdTestResultFileReadWrite(fRc = False) ],
                # Open a file which must exist.
                [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = 'C:\\Windows\\System32\\kernel32.dll', \
                                      sOpenMode = 'r', sDisposition = 'oe'),
                  tdTestResultFileReadWrite(fRc = True) ],
                # Try truncating a file which already is opened with a different sharing mode (and thus should fail).
                [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = 'C:\\Windows\\System32\\kernel32.dll', \
                                      sOpenMode = 'w', sDisposition = 'ot'),
                  tdTestResultFileReadWrite(fRc = False) ]
            ]);

        if oTestVm.sKind == "WindowsXP":
            aaTests.extend([
                # Reading from beginning.
                [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = 'C:\\Windows\\System32\\eula.txt', \
                                      sOpenMode = 'r', sDisposition = 'oe', cbToReadWrite = 33),
                  tdTestResultFileReadWrite(fRc = True, aBuf = 'Microsoft Windows XP Professional', \
                                            cbProcessed = 33, cbOffset = 33) ],
                # Reading from offset.
                [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = 'C:\\Windows\\System32\\eula.txt', \
                                      sOpenMode = 'r', sDisposition = 'oe', cbOffset = 17782, cbToReadWrite = 26),
                  tdTestResultFileReadWrite(fRc = True, aBuf = 'LINKS TO THIRD PARTY SITES', \
                                            cbProcessed = 26, cbOffset = 17782 + 26) ]
            ]);

        fRc = True;
        for (i, aTest) in enumerate(aaTests):
            curTest = aTest[0]; # tdTestFileReadWrite, use an index, later.
            curRes  = aTest[1]; # tdTestResult
            reporter.log('Testing #%d, sFile="%s", cbToReadWrite=%d, sOpenMode="%s", sDisposition="%s", cbOffset=%ld ...' % \
                         (i, curTest.sFile, curTest.cbToReadWrite, curTest.sOpenMode, curTest.sDisposition, curTest.cbOffset));
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            fRc, curGuestSession = curTest.createSession('testGuestCtrlFileRead: Test #%d' % (i,));
            if fRc is False:
                reporter.error('Test #%d failed: Could not create session' % (i,));
                break;
            try:
                if curTest.cbOffset > 0:
                    curFile = curGuestSession.fileOpenEx(curTest.sFile, curTest.sOpenMode, curTest.sDisposition, \
                                                         curTest.sSharingMode, curTest.lCreationMode, curTest.cbOffset);
                    curOffset = long(curFile.offset);
                    resOffset = long(curTest.cbOffset);
                    if curOffset != resOffset:
                        reporter.error('Test #%d failed: Initial offset on open does not match: Got %ld, expected %ld' \
                                       % (i, curOffset, resOffset));
                        fRc = False;
                else:
                    curFile = curGuestSession.fileOpen(curTest.sFile, curTest.sOpenMode, curTest.sDisposition, \
                                                       curTest.lCreationMode);
                if  fRc \
                and curTest.cbToReadWrite > 0:
                    ## @todo Split this up in 64K reads. Later.
                    ## @todo Test timeouts.
                    aBufRead = curFile.read(curTest.cbToReadWrite, 30 * 1000);
                    if  curRes.cbProcessed > 0 \
                    and curRes.cbProcessed is not len(aBufRead):
                        reporter.error('Test #%d failed: Read buffer length does not match: Got %ld, expected %ld' \
                                       % (i, len(aBufRead), curRes.cbProcessed));
                        fRc = False;
                    if fRc:
                        if  curRes.aBuf is not None \
                        and bytes(curRes.aBuf) != bytes(aBufRead):
                            reporter.error('Test #%d failed: Got buffer\n%s (%ld bytes), expected\n%s (%ld bytes)' \
                                           % (i, map(hex, map(ord, aBufRead)), len(aBufRead), \
                                              map(hex, map(ord, curRes.aBuf)), len(curRes.aBuf)));
                            reporter.error('Test #%d failed: Got buffer\n%s, expected\n%s' \
                                           % (i, aBufRead, curRes.aBuf));
                            fRc = False;
                # Test final offset.
                curOffset = long(curFile.offset);
                resOffset = long(curRes.cbOffset);
                if curOffset != resOffset:
                    reporter.error('Test #%d failed: Final offset does not match: Got %ld, expected %ld' \
                                   % (i, curOffset, resOffset));
                    fRc = False;
                curFile.close();
            except:
                reporter.logXcpt('Opening "%s" failed:' % (curTest.sFile,));
                fRc = False;

            curTest.closeSession();

            if fRc != curRes.fRc:
                reporter.error('Test #%d failed: Got %s, expected %s' % (i, fRc, curRes.fRc));
                fRc = False;
                break;

        return (fRc, oTxsSession);

    def testGuestCtrlFileWrite(self, oSession, oTxsSession, oTestVm): # pylint: disable=R0914
        """
        Tests writing to guest files.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
        else:
            sUser = "vbox";
        sPassword = "password";

        if oTestVm.isWindows():
            sScratch = "C:\\Temp\\vboxtest\\testGuestCtrlFileWrite\\";

        if oTxsSession.syncMkDir('${SCRATCH}/testGuestCtrlFileWrite') is False:
            reporter.error('Could not create scratch directory on guest');
            return (False, oTxsSession);

        aaTests = [];

        cScratchBuf = 512;
        aScratchBuf = array('b', [random.randint(-128, 127) for i in range(cScratchBuf)]);
        aaTests.extend([
            # Write to a non-existing file.
            [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = sScratch + 'testGuestCtrlFileWrite.txt', \
                                  sOpenMode = 'w+', sDisposition = 'ce', cbToReadWrite = cScratchBuf,
                                  aBuf = aScratchBuf),
              tdTestResultFileReadWrite(fRc = True, aBuf = aScratchBuf, \
                                        cbProcessed = cScratchBuf, cbOffset = cScratchBuf) ]
        ]);

        aScratchBuf2 = array('b', [random.randint(-128, 127) for i in range(cScratchBuf)]);
        aaTests.extend([
            # Append the same amount of data to the just created file.
            [ tdTestFileReadWrite(sUser = sUser, sPassword = sPassword, sFile = sScratch + 'testGuestCtrlFileWrite.txt', \
                                  sOpenMode = 'w+', sDisposition = 'oa', cbToReadWrite = cScratchBuf,
                                  cbOffset = cScratchBuf, aBuf = aScratchBuf2),
              tdTestResultFileReadWrite(fRc = True, aBuf = aScratchBuf2, \
                                        cbProcessed = cScratchBuf, cbOffset = cScratchBuf * 2) ],
        ]);

        fRc = True;
        for (i, aTest) in enumerate(aaTests):
            curTest = aTest[0]; # tdTestFileReadWrite, use an index, later.
            curRes  = aTest[1]; # tdTestResult
            reporter.log('Testing #%d, sFile="%s", cbToReadWrite=%d, sOpenMode="%s", sDisposition="%s", cbOffset=%ld ...' % \
                         (i, curTest.sFile, curTest.cbToReadWrite, curTest.sOpenMode, curTest.sDisposition, curTest.cbOffset));
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            fRc, curGuestSession = curTest.createSession('testGuestCtrlFileWrite: Test #%d' % (i,));
            if fRc is False:
                reporter.error('Test #%d failed: Could not create session' % (i,));
                break;
            try:
                if curTest.cbOffset > 0:
                    curFile = curGuestSession.fileOpenEx(curTest.sFile, curTest.sOpenMode, curTest.sDisposition, \
                                                         curTest.sSharingMode, curTest.lCreationMode, curTest.cbOffset);
                    curOffset = long(curFile.offset);
                    resOffset = long(curTest.cbOffset);
                    if curOffset != resOffset:
                        reporter.error('Test #%d failed: Initial offset on open does not match: Got %ld, expected %ld' \
                                       % (i, curOffset, resOffset));
                        fRc = False;
                else:
                    curFile = curGuestSession.fileOpen(curTest.sFile, curTest.sOpenMode, curTest.sDisposition, \
                                                       curTest.lCreationMode);
                if  fRc \
                and curTest.cbToReadWrite > 0:
                    ## @todo Split this up in 64K writes. Later.
                    ## @todo Test timeouts.
                    cBytesWritten = curFile.write(curTest.aBuf, 30 * 1000);
                    if  curRes.cbProcessed > 0 \
                    and curRes.cbProcessed != cBytesWritten:
                        reporter.error('Test #%d failed: Written buffer length does not match: Got %ld, expected %ld' \
                                       % (i, cBytesWritten, curRes.cbProcessed));
                        fRc = False;
                    if fRc:
                        # Verify written content by seeking back to the initial offset and
                        # re-read & compare the written data.
                        try:
                            curFile.seek(-(curTest.cbToReadWrite), vboxcon.FileSeekType_Current);
                        except:
                            reporter.logXcpt('Seeking back to initial write position failed:');
                            fRc = False;
                        if  fRc \
                        and long(curFile.offset) != curTest.cbOffset:
                            reporter.error('Test #%d failed: Initial write position does not match current position, \
                                           got %ld, expected %ld' \
                                           % (i, long(curFile.offset), curTest.cbOffset));
                            fRc = False;
                    if fRc:
                        aBufRead = curFile.read(curTest.cbToReadWrite, 30 * 1000);
                        if len(aBufRead) != curTest.cbToReadWrite:
                            reporter.error('Test #%d failed: Got buffer length %ld, expected %ld' \
                                           % (i, len(aBufRead), curTest.cbToReadWrite));
                            fRc = False;
                        if  fRc \
                        and curRes.aBuf is not None \
                        and buffer(curRes.aBuf) != aBufRead:
                            reporter.error('Test #%d failed: Got buffer\n%s, expected\n%s' \
                                           % (i, aBufRead, curRes.aBuf));
                            fRc = False;
                # Test final offset.
                curOffset = long(curFile.offset);
                resOffset = long(curRes.cbOffset);
                if curOffset != resOffset:
                    reporter.error('Test #%d failed: Final offset does not match: Got %ld, expected %ld' \
                                   % (i, curOffset, resOffset));
                    fRc = False;
                curFile.close();
            except:
                reporter.logXcpt('Opening "%s" failed:' % (curTest.sFile,));
                fRc = False;

            curTest.closeSession();

            if fRc != curRes.fRc:
                reporter.error('Test #%d failed: Got %s, expected %s' % (i, fRc, curRes.fRc));
                fRc = False;
                break;

        return (fRc, oTxsSession);

    def testGuestCtrlCopyTo(self, oSession, oTxsSession, oTestVm):
        """
        Tests copying files from host to the guest.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
        else:
            sUser = "vbox";
        sPassword = "password";

        if oTestVm.isWindows():
            sScratch = "C:\\Temp\\vboxtest\\testGuestCtrlCopyTo\\";

        if oTxsSession.syncMkDir('${SCRATCH}/testGuestCtrlCopyTo') is False:
            reporter.error('Could not create scratch directory on guest');
            return (False, oTxsSession);

        # Some stupid trickery to guess the location of the iso.
        sVBoxValidationKitISO = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../VBoxValidationKit.iso'));
        if not os.path.isfile(sVBoxValidationKitISO):
            sVBoxValidationKitISO = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../VBoxTestSuite.iso'));
        if not os.path.isfile(sVBoxValidationKitISO):
            sCur = os.getcwd();
            for i in range(0, 10):
                sVBoxValidationKitISO = os.path.join(sCur, 'validationkit/VBoxValidationKit.iso');
                if os.path.isfile(sVBoxValidationKitISO):
                    break;
                sVBoxValidationKitISO = os.path.join(sCur, 'testsuite/VBoxTestSuite.iso');
                if os.path.isfile(sVBoxValidationKitISO):
                    break;
                sCur = os.path.abspath(os.path.join(sCur, '..'));
                if i is None: pass; # shut up pychecker/pylint.
        if os.path.isfile(sVBoxValidationKitISO):
            reporter.log('Validation Kit .ISO found at: %s' % (sVBoxValidationKitISO,));
        else:
            reporter.log('Warning: Validation Kit .ISO not found -- some tests might fail');

        aaTests = [];
        if oTestVm.isWindows():
            aaTests.extend([
                # Destination missing.
                [ tdTestCopyTo(sUser = sUser, sPassword = sPassword, sSrc = ''),                   tdTestResult(fRc = False) ],
                [ tdTestCopyTo(sUser = sUser, sPassword = sPassword, sSrc = 'C:\\Windows',
                               aFlags = [ 1234 ] ),                                                tdTestResult(fRc = False) ],
                # Source missing.
                [ tdTestCopyTo(sUser = sUser, sPassword = sPassword, sDst = ''),                   tdTestResult(fRc = False) ],
                [ tdTestCopyTo(sUser = sUser, sPassword = sPassword, sDst = 'C:\\Windows',
                               aFlags = [ 1234 ] ),                                                tdTestResult(fRc = False) ],
                # Nothing to copy (source and/or destination is empty).
                [ tdTestCopyTo(sUser = sUser, sPassword = sPassword, sSrc = 'z:\\'),               tdTestResult(fRc = False) ],
                [ tdTestCopyTo(sUser = sUser, sPassword = sPassword, sSrc = '\\\\uncrulez\\foo'),
                                                                                                   tdTestResult(fRc = False) ],
                [ tdTestCopyTo(sUser = sUser, sPassword = sPassword, sSrc = 'non-exist',
                               sDst = os.path.join(sScratch, 'non-exist.dll')),                    tdTestResult(fRc = False) ],
                # Copying single files.
                [ tdTestCopyTo(sUser = sUser, sPassword = sPassword, sSrc = sVBoxValidationKitISO,
                               sDst = 'C:\\non-exist\\'),                                          tdTestResult(fRc = False) ],
                [ tdTestCopyTo(sUser = sUser, sPassword = sPassword, sSrc = sVBoxValidationKitISO,
                               sDst = 'C:\\non\\exist\\'),                                         tdTestResult(fRc = False) ],
                [ tdTestCopyTo(sUser = sUser, sPassword = sPassword, sSrc = sVBoxValidationKitISO,
                               sDst = 'C:\\non\\exist\\renamedfile.dll'),                          tdTestResult(fRc = False) ],
                [ tdTestCopyTo(sUser = sUser, sPassword = sPassword, sSrc = sVBoxValidationKitISO,
                               sDst = os.path.join(sScratch, 'HostGuestAdditions.iso')),           tdTestResult(fRc = True) ],
                [ tdTestCopyTo(sUser = sUser, sPassword = sPassword, sSrc = sVBoxValidationKitISO,
                               sDst = os.path.join(sScratch, 'HostGuestAdditions.iso')),           tdTestResult(fRc = True) ],
                # Destination is a directory, should fail.
                [ tdTestCopyTo(sUser = sUser, sPassword = sPassword, sSrc = sVBoxValidationKitISO,
                               sDst = sScratch),                                                   tdTestResult(fRc = False) ]
                ## @todo Add testing the CopyTo flags here!
            ]);

            if self.oTstDrv.sHost == 'win':
                ## @todo Check for Windows (7) host.
                aaTests.extend([
                    # Copying directories with contain files we don't have read access to.
                    [ tdTestCopyTo(sUser = sUser, sPassword = sPassword, sSrc = 'C:\\Windows\\security',
                                   sDst = sScratch),                                               tdTestResult(fRc = False) ],
                    # Copying directories with regular files.
                    [ tdTestCopyTo(sUser = sUser, sPassword = sPassword, sSrc = 'C:\\Windows\\Help',
                                   sDst = sScratch),                                               tdTestResult(fRc = True) ]
                ]);
        else:
            reporter.log('No OS-specific tests for non-Windows yet!');

        fRc = True;
        for (i, aTest) in enumerate(aaTests):
            curTest = aTest[0]; # tdTestExec, use an index, later.
            curRes  = aTest[1]; # tdTestResult
            reporter.log('Testing #%d, sSrc="%s", sDst="%s", aFlags="%s" ...' % \
                         (i, curTest.sSrc, curTest.sDst, curTest.aFlags));
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            fRc, curGuestSession = curTest.createSession('testGuestCtrlCopyTo: Test #%d' % (i,));
            if fRc is False:
                reporter.error('Test #%d failed: Could not create session' % (i,));
                break;
            fRc2 = self.gctrlCopyTo(curTest, curGuestSession);
            curTest.closeSession();
            if fRc2 is curRes.fRc:
                ## @todo Verify the copied results (size, checksum?).
                pass;
            else:
                reporter.error('Test #%d failed: Got %s, expected %s' % (i, fRc2, curRes.fRc));
                fRc = False;
                break;

        return (fRc, oTxsSession);

    def testGuestCtrlCopyFrom(self, oSession, oTxsSession, oTestVm): # pylint: disable=R0914
        """
        Tests copying files from guest to the host.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
        else:
            sUser = "vbox";
        sPassword = "password";

        sScratch = os.path.join(self.oTstDrv.sScratchPath, "testGctrlCopyFrom");
        try:
            os.makedirs(sScratch);
        except OSError as e:
            if e.errno != errno.EEXIST:
                reporter.error('Failed: Unable to create scratch directory \"%s\"' % (sScratch,));
                return (False, oTxsSession);
        reporter.log('Scratch path is: %s' % (sScratch,));

        aaTests = [];
        if oTestVm.isWindows():
            aaTests.extend([
                # Destination missing.
                [ tdTestCopyFrom(sUser = sUser, sPassword = sPassword, sSrc = ''),
                  tdTestResult(fRc = False) ],
                [ tdTestCopyFrom(sUser = sUser, sPassword = sPassword, sSrc = 'C:\\Windows',
                                 aFlags = [ 1234 ] ),
                  tdTestResult(fRc = False) ],
                # Source missing.
                [ tdTestCopyFrom(sUser = sUser, sPassword = sPassword, sDst = ''),
                  tdTestResult(fRc = False) ],
                [ tdTestCopyFrom(sUser = sUser, sPassword = sPassword, sDst = 'C:\\Windows',
                               aFlags = [ 1234 ] ),
                  tdTestResult(fRc = False) ],
                # Nothing to copy (sDst is empty / unreachable).
                [ tdTestCopyFrom(sUser = sUser, sPassword = sPassword, sSrc = 'z:\\'),
                  tdTestResult(fRc = False) ],
                [ tdTestCopyFrom(sUser = sUser, sPassword = sPassword, sSrc = '\\\\uncrulez\\foo'),

                  tdTestResult(fRc = False) ],
                [ tdTestCopyFrom(sUser = sUser, sPassword = sPassword, sSrc = 'non-exist',
                               sDst = os.path.join(sScratch, 'non-exist.dll')),
                  tdTestResult(fRc = False) ]
                ## @todo Add testing the CopyFrom aFlags here!
            ]);

            if self.oTstDrv.sHost == 'win':
                aaTests.extend([
                    # FIXME: Failing test.
                    # Copying single files.
                    # [ tdTestCopyFrom(sUser = sUser, sPassword = sPassword, sSrc = 'C:\\Windows\\system32\\ole32.dll',
                    #                sDst = 'C:\\non-exist\\'), tdTestResult(fRc = False) ],
                    # [ tdTestCopyFrom(sUser = sUser, sPassword = sPassword, sSrc = 'C:\\Windows\\system32\\ole32.dll',
                    #                sDst = 'C:\\non\\exist\\'), tdTestResult(fRc = False) ],
                    # [ tdTestCopyFrom(sUser = sUser, sPassword = sPassword, sSrc = 'C:\\Windows\\system32\\ole32.dll',
                    #                sDst = 'C:\\non\\exist\\renamedfile.dll'), tdTestResult(fRc = False) ],
                    # [ tdTestCopyFrom(sUser = sUser, sPassword = sPassword, sSrc = 'C:\\Windows\\system32\\ole32.dll',
                    #                sDst = os.path.join(sScratch, 'renamedfile.dll')), tdTestResult(fRc = True) ],
                    # [ tdTestCopyFrom(sUser = sUser, sPassword = sPassword, sSrc = 'C:\\Windows\\system32\\ole32.dll',
                    #                sDst = os.path.join(sScratch, 'renamedfile.dll')), tdTestResult(fRc = True) ],
                    # Destination is a directory, should fail.
                    [ tdTestCopyFrom(sUser = sUser, sPassword = sPassword, sSrc = 'C:\\Windows\\system32\\ole32.dll',
                                   sDst = sScratch),                                                 tdTestResult(fRc = False) ],
                    # Copying directories.
                    [ tdTestCopyFrom(sUser = sUser, sPassword = sPassword, sSrc = 'C:\\Windows\\Web',
                                   sDst = sScratch),                                                 tdTestResult(fRc = True) ]
                    ## @todo Add testing the CopyFrom aFlags here!
                ]);
        else:
            reporter.log('No OS-specific tests for non-Windows yet!');

        fRc = True;
        for (i, aTest) in enumerate(aaTests):
            curTest = aTest[0]; # tdTestExec, use an index, later.
            curRes  = aTest[1]; # tdTestResult
            reporter.log('Testing #%d, sSrc="%s", sDst="%s", aFlags="%s" ...' % \
                         (i, curTest.sSrc, curTest.sDst, curTest.aFlags));
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            fRc, curGuestSession = curTest.createSession('testGuestCtrlCopyFrom: Test #%d' % (i,));
            if fRc is False:
                reporter.error('Test #%d failed: Could not create session' % (i,));
                break;
            fRc2 = self.gctrlCopyFrom(curTest, curRes, curGuestSession);
            curTest.closeSession();
            if fRc2 is curRes.fRc:
                ## @todo Verify the copied results (size, checksum?).
                pass;
            else:
                reporter.error('Test #%d failed: Got %s, expected %s' % (i, fRc2, curRes.fRc));
                fRc = False;
                break;

        return (fRc, oTxsSession);

    def testGuestCtrlUpdateAdditions(self, oSession, oTxsSession, oTestVm): # pylint: disable=R0914
        """
        Tests updating the Guest Additions inside the guest.
        """

        if oTestVm.isWindows():
            sUser = "Administrator";
        else:
            sUser = "vbox";
        sPassword = "password";

        # Some stupid trickery to guess the location of the iso.
        sVBoxValidationKitISO = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../VBoxValidationKit.iso'));
        if not os.path.isfile(sVBoxValidationKitISO):
            sVBoxValidationKitISO = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../VBoxTestSuite.iso'));
        if not os.path.isfile(sVBoxValidationKitISO):
            sCur = os.getcwd();
            for i in range(0, 10):
                sVBoxValidationKitISO = os.path.join(sCur, 'validationkit/VBoxValidationKit.iso');
                if os.path.isfile(sVBoxValidationKitISO):
                    break;
                sVBoxValidationKitISO = os.path.join(sCur, 'testsuite/VBoxTestSuite.iso');
                if os.path.isfile(sVBoxValidationKitISO):
                    break;
                sCur = os.path.abspath(os.path.join(sCur, '..'));
                if i is None: pass; # shut up pychecker/pylint.
        if os.path.isfile(sVBoxValidationKitISO):
            reporter.log('Validation Kit .ISO found at: %s' % (sVBoxValidationKitISO,));
        else:
            reporter.log('Warning: Validation Kit .ISO not found -- some tests might fail');

        sScratch = os.path.join(self.oTstDrv.sScratchPath, "testGctrlUpdateAdditions");
        try:
            os.makedirs(sScratch);
        except OSError as e:
            if e.errno != errno.EEXIST:
                reporter.error('Failed: Unable to create scratch directory \"%s\"' % (sScratch,));
                return (False, oTxsSession);
        reporter.log('Scratch path is: %s' % (sScratch,));

        aaTests = [];
        if oTestVm.isWindows():
            aaTests.extend([
                # Source is missing.
                [ tdTestUpdateAdditions(sUser = sUser, sPassword = sPassword, sSrc = ''),
                  tdTestResult(fRc = False) ],
                # Wrong aFlags.
                [ tdTestUpdateAdditions(sUser = sUser, sPassword = sPassword, sSrc = self.oTstDrv.getGuestAdditionsIso(),
                                        aFlags = [ 1234 ]),
                  tdTestResult(fRc = False) ],
                # Non-existing .ISO.
                [ tdTestUpdateAdditions(sUser = sUser, sPassword = sPassword, sSrc = "non-existing.iso"),
                  tdTestResult(fRc = False) ],
                # Wrong .ISO.
                [ tdTestUpdateAdditions(sUser = sUser, sPassword = sPassword, sSrc = sVBoxValidationKitISO),
                  tdTestResult(fRc = False) ],
                # The real thing.
                [ tdTestUpdateAdditions(sUser = sUser, sPassword = sPassword, sSrc = self.oTstDrv.getGuestAdditionsIso()),
                  tdTestResult(fRc = True) ],
                # Test the (optional) installer arguments. This will extract the
                # installer into our guest's scratch directory.
                [ tdTestUpdateAdditions(sUser = sUser, sPassword = sPassword, sSrc = self.oTstDrv.getGuestAdditionsIso(),
                                        aArgs = [ '/extract', '/D=' + sScratch ]),
                  tdTestResult(fRc = True) ]
                # Some debg ISO. Only enable locally.
                #[ tdTestUpdateAdditions(sUser = sUser, sPassword = sPassword,
                #                      sSrc = "V:\\Downloads\\VBoxGuestAdditions-r80354.iso"),
                #  tdTestResult(fRc = True) ]
            ]);
        else:
            reporter.log('No OS-specific tests for non-Windows yet!');

        fRc = True;
        for (i, aTest) in enumerate(aaTests):
            curTest = aTest[0]; # tdTestExec, use an index, later.
            curRes  = aTest[1]; # tdTestResult
            reporter.log('Testing #%d, sSrc="%s", aFlags="%s" ...' % \
                         (i, curTest.sSrc, curTest.aFlags));
            curTest.setEnvironment(oSession, oTxsSession, oTestVm);
            fRc, _ = curTest.createSession('Test #%d' % (i,));
            if fRc is False:
                reporter.error('Test #%d failed: Could not create session' % (i,));
                break;
            try:
                curProgress = curTest.oTest.oGuest.updateGuestAdditions(curTest.sSrc, curTest.aArgs, curTest.aFlags);
                if curProgress is not None:
                    oProgress = vboxwrappers.ProgressWrapper(curProgress, self.oTstDrv.oVBoxMgr, self, "gctrlUpGA");
                    try:
                        iRc = oProgress.waitForOperation(0, fIgnoreErrors = True);
                        if iRc != 0:
                            reporter.log('Waiting for updating Guest Additions failed');
                            fRc = False;
                    except:
                        reporter.logXcpt('Updating Guest Additions waiting exception for sSrc="%s", aFlags="%s":' \
                                         % (curTest.sSrc, curTest.aFlags));
                        fRc = False;
            except:
                # Just log, don't assume an error here (will be done in the main loop then).
                reporter.logXcpt('Updating Guest Additions exception for sSrc="%s", aFlags="%s":' \
                                 % (curTest.sSrc, curTest.aFlags));
                fRc = False;
            curTest.closeSession();
            if fRc is curRes.fRc:
                if fRc:
                    ## @todo Verify if Guest Additions were really updated (build, revision, ...).
                    pass;
            else:
                reporter.error('Test #%d failed: Got %s, expected %s' % (i, fRc, curRes.fRc));
                fRc = False;
                break;

        return (fRc, oTxsSession);



class tdAddGuestCtrl(vbox.TestDriver):                                         # pylint: disable=R0902,R0904
    """
    Guest control using VBoxService on the guest.
    """

    def __init__(self):
        vbox.TestDriver.__init__(self);
        self.oTestVmSet = self.oTestVmManager.getStandardVmSet('nat');
        self.fQuick     = False; # Don't skip lengthly tests by default.
        self.addSubTestDriver(SubTstDrvAddGuestCtrl(self));

    #
    # Overridden methods.
    #
    def showUsage(self):
        """
        Shows the testdriver usage.
        """
        rc = vbox.TestDriver.showUsage(self);
        reporter.log('');
        reporter.log('tdAddGuestCtrl Options:');
        reporter.log('  --quick');
        reporter.log('      Same as --virt-modes hwvirt --cpu-counts 1.');
        return rc;

    def parseOption(self, asArgs, iArg):                                        # pylint: disable=R0912,R0915
        """
        Parses the testdriver arguments from the command line.
        """
        if asArgs[iArg] == '--quick':
            self.parseOption(['--virt-modes', 'hwvirt'], 0);
            self.parseOption(['--cpu-counts', '1'], 0);
            self.fQuick = True;
        else:
            return vbox.TestDriver.parseOption(self, asArgs, iArg);
        return iArg + 1;

    def actionConfig(self):
        if not self.importVBoxApi(): # So we can use the constant below.
            return False;

        eNic0AttachType = vboxcon.NetworkAttachmentType_NAT;
        sGaIso = self.getGuestAdditionsIso();
        return self.oTestVmSet.actionConfig(self, eNic0AttachType = eNic0AttachType, sDvdImage = sGaIso);

    def actionExecute(self):
        return self.oTestVmSet.actionExecute(self, self.testOneCfg);

    #
    # Test execution helpers.
    #
    def testOneCfg(self, oVM, oTestVm): # pylint: disable=R0915
        """
        Runs the specified VM thru the tests.

        Returns a success indicator on the general test execution. This is not
        the actual test result.
        """

        self.logVmInfo(oVM);

        fRc = True;
        oSession, oTxsSession = self.startVmAndConnectToTxsViaTcp(oTestVm.sVmName, fCdWait = False);
        reporter.log("TxsSession: %s" % (oTxsSession,));
        if oSession is not None:
            self.addTask(oSession);

            fManual = False; # Manual override for local testing. (Committed version shall be False.)
            if not fManual:
                fRc, oTxsSession = self.aoSubTstDrvs[0].testIt(oTestVm, oSession, oTxsSession);
            else:
                fRc, oTxsSession = self.testGuestCtrlManual(oSession, oTxsSession, oTestVm);

            # Cleanup.
            self.removeTask(oTxsSession);
            if not fManual:
                self.terminateVmBySession(oSession);
        else:
            fRc = False;
        return fRc;

    def gctrlReportError(self, progress):
        """
        Helper function to report an error of a
        given progress object.
        """
        if progress is None:
            reporter.log('No progress object to print error for');
        else:
            errInfo = progress.errorInfo;
            if errInfo:
                reporter.log('%s' % (errInfo.text,));
        return False;

    def gctrlGetRemainingTime(self, msTimeout, msStart):
        """
        Helper function to return the remaining time (in ms)
        based from a timeout value and the start time (both in ms).
        """
        if msTimeout is 0:
            return 0xFFFFFFFE; # Wait forever.
        msElapsed = base.timestampMilli() - msStart;
        if msElapsed > msTimeout:
            return 0; # No time left.
        return msTimeout - msElapsed;

    def testGuestCtrlManual(self, oSession, oTxsSession, oTestVm):                # pylint: disable=R0914,R0915,W0613,W0612
        """
        For manually testing certain bits.
        """

        reporter.log('Manual testing ...');
        fRc = True;

        sUser = 'Administrator';
        sPassword = 'password';

        oGuest = oSession.o.console.guest;
        oGuestSession = oGuest.createSession(sUser,
                                             sPassword,
                                             "", "Manual Test");

        aWaitFor = [ vboxcon.GuestSessionWaitForFlag_Start ];
        _ = oGuestSession.waitForArray(aWaitFor, 30 * 1000);

        sCmd = 'c:\\windows\\system32\\cmd.exe';
        aArgs = [ '/C', 'dir', '/S', 'c:\\windows' ];
        aEnv = [];
        aFlags = [];

        for _ in range(100):
            oProc = oGuestSession.processCreate(sCmd,
                                                aArgs, aEnv,
                                                aFlags, 30 * 1000);

            aWaitFor = [ vboxcon.ProcessWaitForFlag_Terminate ];
            _ = oProc.waitForArray(aWaitFor, 30 * 1000);

        oGuestSession.close();
        oGuestSession = None;

        time.sleep(5);

        oSession.o.console.PowerDown();

        return (fRc, oTxsSession);

if __name__ == '__main__':
    sys.exit(tdAddGuestCtrl().main(sys.argv));

