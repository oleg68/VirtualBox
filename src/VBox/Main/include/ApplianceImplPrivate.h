/* $Id$ */
/** @file
 * VirtualBox Appliance private data definitions
 */

/*
 * Copyright (C) 2006-2013 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_APPLIANCEIMPLPRIVATE
#define ____H_APPLIANCEIMPLPRIVATE

class VirtualSystemDescription;

#include "ovfreader.h"
#include "SecretKeyStore.h"
#include "ThreadTask.h"
#include <map>
#include <vector>
#include <iprt/manifest.h>
#include <iprt/vfs.h>
#include <iprt/crypto/x509.h>

////////////////////////////////////////////////////////////////////////////////
//
// Appliance data definition
//
////////////////////////////////////////////////////////////////////////////////

typedef std::pair<Utf8Str, Utf8Str> STRPAIR;

typedef std::vector<com::Guid> GUIDVEC;

/* Describe a location for the import/export. The location could be a file on a
 * local hard disk or a remote target based on the supported inet protocols. */
struct LocationInfo
{
    LocationInfo()
      : storageType(VFSType_File) {}
    VFSType_T storageType; /* Which type of storage should be handled */
    Utf8Str strPath;       /* File path for the import/export */
    Utf8Str strHostname;   /* Hostname on remote storage locations (could be empty) */
    Utf8Str strUsername;   /* Username on remote storage locations (could be empty) */
    Utf8Str strPassword;   /* Password on remote storage locations (could be empty) */
};

// opaque private instance data of Appliance class
struct Appliance::Data
{
    enum ApplianceState { ApplianceIdle, ApplianceImporting, ApplianceExporting };
    enum digest_T {SHA1, SHA256};

    Data()
      : state(ApplianceIdle)
      , fDigestTypes(0)
      , hOurManifest(NIL_RTMANIFEST)
      , fManifest(true)
      , fSha256(false)
      , fDeterminedDigestTypes(false)
      , hTheirManifest(NIL_RTMANIFEST)
      , hMemFileTheirManifest(NIL_RTVFSFILE)
      , fSignerCertLoaded(false)
      , fSignatureValid(false)
      , pbSignedDigest(NULL)
      , cbSignedDigest(0)
      , enmSignedDigestType(RTDIGESTTYPE_INVALID)
      , fExportISOImages(false)
      , pReader(NULL)
      , ulWeightForXmlOperation(0)
      , ulWeightForManifestOperation(0)
      , ulTotalDisksMB(0)
      , cDisks(0)
      , m_cPwProvided(0)
    {
    }

    ~Data()
    {
        if (pReader)
        {
            delete pReader;
            pReader = NULL;
        }
        resetReadData();
    }

    /**
     * Resets data used by read.
     */
    void resetReadData(void)
    {
        strOvfManifestEntry.setNull();
        if (hOurManifest != NIL_RTMANIFEST)
        {
            RTManifestRelease(hOurManifest);
            hOurManifest = NIL_RTMANIFEST;
        }
        if (hTheirManifest != NIL_RTMANIFEST)
        {
            RTManifestRelease(hTheirManifest);
            hTheirManifest = NIL_RTMANIFEST;
        }
        if (hMemFileTheirManifest)
        {
            RTVfsFileRelease(hMemFileTheirManifest);
            hMemFileTheirManifest = NIL_RTVFSFILE;
        }
        if (pbSignedDigest)
        {
            RTMemFree(pbSignedDigest);
            pbSignedDigest = NULL;
            cbSignedDigest = 0;
        }
        if (fSignerCertLoaded)
        {
            RTCrX509Certificate_Delete(&SignerCert);
            fSignerCertLoaded = false;
        }
        enmSignedDigestType    = RTDIGESTTYPE_INVALID;
        fSignatureValid        = false;
        fDeterminedDigestTypes = false;
        fDigestTypes           = RTMANIFEST_ATTR_SHA1 | RTMANIFEST_ATTR_SHA256;
    }

    ApplianceState      state;

    LocationInfo        locInfo;        // location info for the currently processed OVF
    /** The digests types to calculate (RTMANIFEST_ATTR_XXX) for the manifest.
     * This will be a single value when exporting.  Zero, one or two.  */
    uint32_t            fDigestTypes;
    /** Manifest created while importing or exporting. */
    RTMANIFEST          hOurManifest;

    /** @name Write data
     * @{ */
    bool                fManifest;      // Create a manifest file on export
    bool                fSha256;        // true = SHA256 (OVF 2.0), false = SHA1 (OVF 1.0)
    /** @} */

    /** @name Read data
     *  @{ */
    /** The manifest entry name of the OVF-file. */
    Utf8Str             strOvfManifestEntry;

    /** Set if we've parsed the manifest and determined the digest types. */
    bool                fDeterminedDigestTypes;

    /** Manifest read in during read() and kept around for later verification. */
    RTMANIFEST          hTheirManifest;
    /** Memorized copy of the manifest file for signature checking purposes. */
    RTVFSFILE           hMemFileTheirManifest;

    /** The signer certificate from the signature fiel (.cert).
     * This will be used in the future provide information about the signer via
     * the API. */
    RTCRX509CERTIFICATE SignerCert;
    /** Set if the SignerCert member contains usable data. */
    bool                fSignerCertLoaded;
    /** Set by read() if it found a certificate and the signature is fine. */
    bool                fSignatureValid;
    /** The signed digest of the manifest. */
    uint8_t            *pbSignedDigest;
    /** The size of the signed digest. */
    size_t              cbSignedDigest;
    /** The digest type used to sign the manifest. */
    RTDIGESTTYPE        enmSignedDigestType;
    /** @} */

    bool                fExportISOImages;// when 1 the ISO images are exported

    RTCList<ImportOptions_T> optListImport;
    RTCList<ExportOptions_T> optListExport;

    ovf::OVFReader      *pReader;

    std::list< ComObjPtr<VirtualSystemDescription> >
                        virtualSystemDescriptions;

    std::list<Utf8Str>  llWarnings;

    ULONG               ulWeightForXmlOperation;
    ULONG               ulWeightForManifestOperation;
    ULONG               ulTotalDisksMB;
    ULONG               cDisks;

    std::list<Guid>     llGuidsMachinesCreated;

    /** Sequence of password identifiers to encrypt disk images during export. */
    std::vector<com::Utf8Str> m_vecPasswordIdentifiers;
    /** Map to get all medium identifiers assoicated with a given password identifier. */
    std::map<com::Utf8Str, GUIDVEC> m_mapPwIdToMediumIds;
    /** Secret key store used to hold the passwords during export. */
    SecretKeyStore            *m_pSecretKeyStore;
    /** Number of passwords provided. */
    uint32_t                  m_cPwProvided;
};

struct Appliance::XMLStack
{
    std::map<Utf8Str, const VirtualSystemDescriptionEntry*> mapDisks;
    std::map<Utf8Str, bool> mapNetworks;
};

class Appliance::TaskOVF: public ThreadTask
{
public:
    enum TaskType
    {
        Read,
        Import,
        Write
    };

    TaskOVF(Appliance *aThat,
            TaskType aType,
            LocationInfo aLocInfo,
            ComObjPtr<Progress> &aProgress)
      : ThreadTask("TaskOVF"),
        pAppliance(aThat),
        taskType(aType),
        locInfo(aLocInfo),
        pProgress(aProgress),
        enFormat(ovf::OVFVersion_unknown),
        rc(S_OK)
    {
        switch (taskType)
        {
            case TaskOVF::Read:     m_strTaskName = "ApplRead"; break;
            case TaskOVF::Import:   m_strTaskName = "ApplImp"; break;
            case TaskOVF::Write:    m_strTaskName = "ApplWrit"; break;
            default:                m_strTaskName = "ApplTask"; break;
        }
    }

    static DECLCALLBACK(int) updateProgress(unsigned uPercent, void *pvUser);

    Appliance *pAppliance;
    TaskType taskType;
    const LocationInfo locInfo;
    ComObjPtr<Progress> pProgress;

    ovf::OVFVersion_T enFormat;

    HRESULT rc;

    void handler()
    {
        int vrc = Appliance::i_taskThreadImportOrExport(NULL, this); NOREF(vrc);
    }
};

struct MyHardDiskAttachment
{
    ComPtr<IMachine>    pMachine;
    Bstr                controllerType;
    int32_t             lControllerPort;        // 0-29 for SATA
    int32_t             lDevice;                // IDE: 0 or 1, otherwise 0 always
};

/**
 * Used by Appliance::importMachineGeneric() to store
 * input parameters and rollback information.
 */
struct Appliance::ImportStack
{
    // input pointers
    const LocationInfo              &locInfo;           // ptr to location info from Appliance::importFS()
    Utf8Str                         strSourceDir;       // directory where source files reside
    const ovf::DiskImagesMap        &mapDisks;          // ptr to disks map in OVF
    ComObjPtr<Progress>             &pProgress;         // progress object passed into Appliance::importFS()

    // input parameters from VirtualSystemDescriptions
    Utf8Str                         strNameVBox;        // VM name
    Utf8Str                         strMachineFolder;   // FQ host folder where the VirtualBox machine would be created
    Utf8Str                         strOsTypeVBox;      // VirtualBox guest OS type as string
    Utf8Str                         strDescription;
    uint32_t                        cCPUs;              // CPU count
    bool                            fForceHWVirt;       // if true, we force enabling hardware virtualization
    bool                            fForceIOAPIC;       // if true, we force enabling the IOAPIC
    uint32_t                        ulMemorySizeMB;     // virtual machine RAM in megabytes
#ifdef VBOX_WITH_USB
    bool                            fUSBEnabled;
#endif
    Utf8Str                         strAudioAdapter;    // if not empty, then the guest has audio enabled, and this is the decimal
                                                        // representation of the audio adapter (should always be "0" for AC97 presently)

    // session (not initially created)
    ComPtr<ISession>                pSession;           // session opened in Appliance::importFS() for machine manipulation
    bool                            fSessionOpen;       // true if the pSession is currently open and needs closing

    /** @name File access related stuff (TAR stream)
     *  @{  */
    /** OVA file system stream handle. NIL if not OVA.  */
    RTVFSFSSTREAM                   hVfsFssOva;
    /** OVA lookahead I/O stream object. */
    RTVFSIOSTREAM                   hVfsIosOvaLookAhead;
    /** OVA lookahead I/O stream object name. */
    char                           *pszOvaLookAheadName;
    /** @} */

    // a list of images that we created/imported; this is initially empty
    // and will be cleaned up on errors
    std::list<MyHardDiskAttachment> llHardDiskAttachments;      // disks that were attached
    std::map<Utf8Str , Utf8Str>     mapNewUUIDsToOriginalUUIDs;

    ImportStack(const LocationInfo &aLocInfo,
                const ovf::DiskImagesMap &aMapDisks,
                ComObjPtr<Progress> &aProgress,
                RTVFSFSSTREAM aVfsFssOva)
        : locInfo(aLocInfo),
          mapDisks(aMapDisks),
          pProgress(aProgress),
          cCPUs(1),
          fForceHWVirt(false),
          fForceIOAPIC(false),
          ulMemorySizeMB(0),
          fSessionOpen(false),
          hVfsFssOva(aVfsFssOva),
          hVfsIosOvaLookAhead(NIL_RTVFSIOSTREAM),
          pszOvaLookAheadName(NULL)
    {
        if (hVfsFssOva != NIL_RTVFSFSSTREAM)
            RTVfsFsStrmRetain(hVfsFssOva);

        // disk images have to be on the same place as the OVF file. So
        // strip the filename out of the full file path
        strSourceDir = aLocInfo.strPath;
        strSourceDir.stripFilename();
    }

    ~ImportStack()
    {
        if (hVfsFssOva != NIL_RTVFSFSSTREAM)
        {
            RTVfsFsStrmRelease(hVfsFssOva);
            hVfsFssOva = NIL_RTVFSFSSTREAM;
        }
        if (hVfsIosOvaLookAhead != NIL_RTVFSIOSTREAM)
        {
            RTVfsIoStrmRelease(hVfsIosOvaLookAhead);
            hVfsIosOvaLookAhead = NIL_RTVFSIOSTREAM;
        }
        if (pszOvaLookAheadName)
        {
            RTStrFree(pszOvaLookAheadName);
            pszOvaLookAheadName = NULL;
        }
    }

    HRESULT restoreOriginalUUIDOfAttachedDevice(settings::MachineConfigFile *config);
    HRESULT saveOriginalUUIDOfAttachedDevice(settings::AttachedDevice &device,
                                                  const Utf8Str &newlyUuid);
    RTVFSIOSTREAM claimOvaLookAHead(void);

};

////////////////////////////////////////////////////////////////////////////////
//
// VirtualSystemDescription data definition
//
////////////////////////////////////////////////////////////////////////////////

struct VirtualSystemDescription::Data
{
    std::vector<VirtualSystemDescriptionEntry>
                            maDescriptions;     // item descriptions

    ComPtr<Machine>         pMachine;           // VirtualBox machine this description was exported from (export only)

    settings::MachineConfigFile
                            *pConfig;           // machine config created from <vbox:Machine> element if found (import only)
};

////////////////////////////////////////////////////////////////////////////////
//
// Internal helpers
//
////////////////////////////////////////////////////////////////////////////////

void convertCIMOSType2VBoxOSType(Utf8Str &strType, ovf::CIMOSType_T c, const Utf8Str &cStr);

ovf::CIMOSType_T convertVBoxOSType2CIMOSType(const char *pcszVBox, BOOL fLongMode);

Utf8Str convertNetworkAttachmentTypeToString(NetworkAttachmentType_T type);


typedef struct SHASTORAGE
{
    PVDINTERFACE pVDImageIfaces;
    bool         fCreateDigest;
    bool         fSha256;        /* false = SHA1 (OVF 1.x), true = SHA256 (OVF 2.0) */
    Utf8Str      strDigest;
} SHASTORAGE, *PSHASTORAGE;

PVDINTERFACEIO ShaCreateInterface();
PVDINTERFACEIO FileCreateInterface();
PVDINTERFACEIO tarWriterCreateInterface(void);

int writeBufferToFile(const char *pcszFilename, void *pvBuf, size_t cbSize, PVDINTERFACEIO pIfIo, void *pvUser);

#endif // !____H_APPLIANCEIMPLPRIVATE

