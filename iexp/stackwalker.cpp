#include "stackwalker.h"

#ifdef WIN32


#include "stackwalkerinternal.h"
#include "iexp.h"

StackWalker::StackWalker(DWORD dwProcessId, HANDLE hProcess)
{
    this->m_options = OptionsAll;
    this->m_modulesLoaded = FALSE;
    this->m_hProcess = hProcess;
    this->m_sw = new StackWalkerInternal(this, this->m_hProcess);
    this->m_dwProcessId = dwProcessId;
    this->m_szSymPath = NULL;
}

StackWalker::StackWalker(int options, LPCSTR szSymPath, DWORD dwProcessId, HANDLE hProcess)
{
    this->m_options = options;
    this->m_modulesLoaded = FALSE;
    this->m_hProcess = hProcess;
    this->m_sw = new StackWalkerInternal(this, this->m_hProcess);
    this->m_dwProcessId = dwProcessId;
    if (szSymPath != NULL)
    {
        this->m_szSymPath = _strdup(szSymPath);
        this->m_options |= SymBuildPath;
    }
    else
        this->m_szSymPath = NULL;
}

StackWalker::~StackWalker()
{
    if (m_szSymPath != NULL)
        free(m_szSymPath);
    m_szSymPath = NULL;
    if (this->m_sw != NULL)
        delete this->m_sw;
    this->m_sw = NULL;
}

BOOL StackWalker::LoadModules()
{
    if (this->m_sw == NULL)
    {
        SetLastError(ERROR_DLL_INIT_FAILED);
        return FALSE;
    }
    if (m_modulesLoaded != FALSE)
        return TRUE;

    // Build the sym-path:
    char *szSymPath = NULL;
    if ( (this->m_options & SymBuildPath) != 0)
    {
        const size_t nSymPathLen = 4096;
        szSymPath = (char*) malloc(nSymPathLen);
        if (szSymPath == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        szSymPath[0] = 0;
        // Now first add the (optional) provided sympath:
        if (this->m_szSymPath != NULL)
        {
            strcat_s(szSymPath, nSymPathLen, this->m_szSymPath);
            strcat_s(szSymPath, nSymPathLen, ";");
        }

        strcat_s(szSymPath, nSymPathLen, ".;");

        const size_t nTempLen = 1024;
        char szTemp[nTempLen];
        // Now add the current directory:
        if (GetCurrentDirectoryA(nTempLen, szTemp) > 0)
        {
            szTemp[nTempLen-1] = 0;
            strcat_s(szSymPath, nSymPathLen, szTemp);
            strcat_s(szSymPath, nSymPathLen, ";");
        }

        // Now add the path for the main-module:
        if (GetModuleFileNameA(NULL, szTemp, nTempLen) > 0)
        {
            szTemp[nTempLen-1] = 0;
            for (char *p = (szTemp+strlen(szTemp)-1); p >= szTemp; --p)
            {
                // locate the rightmost path separator
                if ( (*p == '\\') || (*p == '/') || (*p == ':') )
                {
                    *p = 0;
                    break;
                }
            }  // for (search for path separator...)
            if (strlen(szTemp) > 0)
            {
                strcat_s(szSymPath, nSymPathLen, szTemp);
                strcat_s(szSymPath, nSymPathLen, ";");
            }
        }
        if (GetEnvironmentVariableA("_NT_SYMBOL_PATH", szTemp, nTempLen) > 0)
        {
            szTemp[nTempLen-1] = 0;
            strcat_s(szSymPath, nSymPathLen, szTemp);
            strcat_s(szSymPath, nSymPathLen, ";");
        }
        if (GetEnvironmentVariableA("_NT_ALTERNATE_SYMBOL_PATH", szTemp, nTempLen) > 0)
        {
            szTemp[nTempLen-1] = 0;
            strcat_s(szSymPath, nSymPathLen, szTemp);
            strcat_s(szSymPath, nSymPathLen, ";");
        }
        if (GetEnvironmentVariableA("SYSTEMROOT", szTemp, nTempLen) > 0)
        {
            szTemp[nTempLen-1] = 0;
            strcat_s(szSymPath, nSymPathLen, szTemp);
            strcat_s(szSymPath, nSymPathLen, ";");
            // also add the "system32"-directory:
            strcat_s(szTemp, nTempLen, "\\system32");
            strcat_s(szSymPath, nSymPathLen, szTemp);
            strcat_s(szSymPath, nSymPathLen, ";");
        }

        if ( (this->m_options & SymBuildPath) != 0)
        {
            if (GetEnvironmentVariableA("SYSTEMDRIVE", szTemp, nTempLen) > 0)
            {
                szTemp[nTempLen-1] = 0;
                strcat_s(szSymPath, nSymPathLen, "SRV*");
                strcat_s(szSymPath, nSymPathLen, szTemp);
                strcat_s(szSymPath, nSymPathLen, "\\websymbols");
                strcat_s(szSymPath, nSymPathLen, "*http://msdl.microsoft.com/download/symbols;");
            }
            else
                strcat_s(szSymPath, nSymPathLen, "SRV*c:\\websymbols*http://msdl.microsoft.com/download/symbols;");
        }
    }

    // First Init the whole stuff...
    BOOL bRet = this->m_sw->Init(szSymPath);
    if (szSymPath != NULL) free(szSymPath); szSymPath = NULL;
    if (bRet == FALSE)
    {
        this->OnDbgHelpErr("Error while initializing dbghelp.dll", 0, 0);
        SetLastError(ERROR_DLL_INIT_FAILED);
        return FALSE;
    }

    bRet = this->m_sw->LoadModules(this->m_hProcess, this->m_dwProcessId);
    if (bRet != FALSE)
        m_modulesLoaded = TRUE;
    return bRet;
}


// The following is used to pass the "userData"-Pointer to the user-provided readMemoryFunction
// This has to be done due to a problem with the "hProcess"-parameter in x64...
// Because this class is in no case multi-threading-enabled (because of the limitations 
// of dbghelp.dll) it is "safe" to use a static-variable
static StackWalker::PReadProcessMemoryRoutine s_readMemoryFunction = NULL;
static LPVOID s_readMemoryFunction_UserData = NULL;

BOOL StackWalker::ShowCallstack(HANDLE hThread, const CONTEXT *context, PReadProcessMemoryRoutine readMemoryFunction, LPVOID pUserData)
{
    CONTEXT c;;
    CallstackEntry csEntry;
    IMAGEHLP_SYMBOL64 *pSym = NULL;
    StackWalkerInternal::IMAGEHLP_MODULE64_V2 Module;
    IMAGEHLP_LINE64 Line;
    int frameNum;

    if (m_modulesLoaded == FALSE)
        this->LoadModules();  // ignore the result...

    if (this->m_sw->m_hDbhHelp == NULL)
    {
        SetLastError(ERROR_DLL_INIT_FAILED);
        return FALSE;
    }

    s_readMemoryFunction = readMemoryFunction;
    s_readMemoryFunction_UserData = pUserData;

    if (context == NULL)
    {
        // If no context is provided, capture the context
        if (hThread == GetCurrentThread())
        {
            memset(&c, 0, sizeof(CONTEXT));
            c.ContextFlags = USED_CONTEXT_FLAGS;
            RtlCaptureContext(&c);
        }
        else
        {
            SuspendThread(hThread);
            memset(&c, 0, sizeof(CONTEXT));
            c.ContextFlags = USED_CONTEXT_FLAGS;
            if (GetThreadContext(hThread, &c) == FALSE)
            {
                ResumeThread(hThread);
                return FALSE;
            }
        }
    }
    else
        c = *context;

    // init STACKFRAME for first call
    STACKFRAME64 s; // in/out stackframe
    memset(&s, 0, sizeof(s));
    DWORD imageType;
#ifdef _M_IX86
    // normally, call ImageNtHeader() and use machine info from PE header
    imageType = IMAGE_FILE_MACHINE_I386;
    s.AddrPC.Offset = c.Eip;
    s.AddrPC.Mode = AddrModeFlat;
    s.AddrFrame.Offset = c.Ebp;
    s.AddrFrame.Mode = AddrModeFlat;
    s.AddrStack.Offset = c.Esp;
    s.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
    imageType = IMAGE_FILE_MACHINE_AMD64;
    s.AddrPC.Offset = c.Rip;
    s.AddrPC.Mode = AddrModeFlat;
    s.AddrFrame.Offset = c.Rsp;
    s.AddrFrame.Mode = AddrModeFlat;
    s.AddrStack.Offset = c.Rsp;
    s.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
    imageType = IMAGE_FILE_MACHINE_IA64;
    s.AddrPC.Offset = c.StIIP;
    s.AddrPC.Mode = AddrModeFlat;
    s.AddrFrame.Offset = c.IntSp;
    s.AddrFrame.Mode = AddrModeFlat;
    s.AddrBStore.Offset = c.RsBSP;
    s.AddrBStore.Mode = AddrModeFlat;
    s.AddrStack.Offset = c.IntSp;
    s.AddrStack.Mode = AddrModeFlat;
#else
#error "Platform not supported!"
#endif

    pSym = (IMAGEHLP_SYMBOL64 *) malloc(sizeof(IMAGEHLP_SYMBOL64) + STACKWALK_MAX_NAMELEN);
    if (!pSym) goto cleanup;  // not enough memory...
    memset(pSym, 0, sizeof(IMAGEHLP_SYMBOL64) + STACKWALK_MAX_NAMELEN);
    pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
    pSym->MaxNameLength = STACKWALK_MAX_NAMELEN;

    memset(&Line, 0, sizeof(Line));
    Line.SizeOfStruct = sizeof(Line);

    memset(&Module, 0, sizeof(Module));
    Module.SizeOfStruct = sizeof(Module);

    for (frameNum = 0; frameNum <= MAX_STACK_WALKER_STEP; ++frameNum )
    {
        // get next stack frame (StackWalk64(), SymFunctionTableAccess64(), SymGetModuleBase64())
        // if this returns ERROR_INVALID_ADDRESS (487) or ERROR_NOACCESS (998), you can
        // assume that either you are done, or that the stack is so hosed that the next
        // deeper frame could not be found.
        // CONTEXT need not to be suplied if imageTyp is IMAGE_FILE_MACHINE_I386!
        if ( ! this->m_sw->pSW(imageType, this->m_hProcess, hThread, &s, &c, myReadProcMem, this->m_sw->pSFTA, this->m_sw->pSGMB, NULL) )
        {
            this->OnDbgHelpErr("StackWalk64", GetLastError(), s.AddrPC.Offset);
            break;
        }

        csEntry.offset = s.AddrPC.Offset;
        csEntry.name[0] = 0;
        csEntry.undName[0] = 0;
        csEntry.undFullName[0] = 0;
        csEntry.offsetFromSmybol = 0;
        csEntry.offsetFromLine = 0;
        csEntry.lineFileName[0] = 0;
        csEntry.lineNumber = 0;
        csEntry.loadedImageName[0] = 0;
        csEntry.moduleName[0] = 0;
        if (s.AddrPC.Offset == s.AddrReturn.Offset)
        {
            this->OnDbgHelpErr("StackWalk64-Endless-Callstack!", 0, s.AddrPC.Offset);
            break;
        }
        if (s.AddrPC.Offset != 0)
        {
            // we seem to have a valid PC
            // show procedure info (SymGetSymFromAddr64())
            if (this->m_sw->pSGSFA(this->m_hProcess, s.AddrPC.Offset, &(csEntry.offsetFromSmybol), pSym) != FALSE)
            {
                // TODO: Mache dies sicher...!
                strcpy_s(csEntry.name, pSym->Name);
                // UnDecorateSymbolName()
                this->m_sw->pUDSN( pSym->Name, csEntry.undName, STACKWALK_MAX_NAMELEN, UNDNAME_NAME_ONLY );
                this->m_sw->pUDSN( pSym->Name, csEntry.undFullName, STACKWALK_MAX_NAMELEN, UNDNAME_COMPLETE );
            }
            else
            {
                this->OnDbgHelpErr("SymGetSymFromAddr64", GetLastError(), s.AddrPC.Offset);
            }

            // show line number info, NT5.0-method (SymGetLineFromAddr64())
            if (this->m_sw->pSGLFA != NULL )
            { // yes, we have SymGetLineFromAddr64()
                if (this->m_sw->pSGLFA(this->m_hProcess, s.AddrPC.Offset, &(csEntry.offsetFromLine), &Line) != FALSE)
                {
                    csEntry.lineNumber = Line.LineNumber;
                    // TODO: Mache dies sicher...!
                    strcpy_s(csEntry.lineFileName, Line.FileName);
                }
                else
                {
                    this->OnDbgHelpErr("SymGetLineFromAddr64", GetLastError(), s.AddrPC.Offset);
                }
            } // yes, we have SymGetLineFromAddr64()

            // show module info (SymGetModuleInfo64())
            if (this->m_sw->GetModuleInfo(this->m_hProcess, s.AddrPC.Offset, &Module ) != FALSE)
            { // got module info OK
                switch ( Module.SymType )
                {
                case SymNone:
                    csEntry.symTypeString = "-nosymbols-";
                    break;
                case SymCoff:
                    csEntry.symTypeString = "COFF";
                    break;
                case SymCv:
                    csEntry.symTypeString = "CV";
                    break;
                case SymPdb:
                    csEntry.symTypeString = "PDB";
                    break;
                case SymExport:
                    csEntry.symTypeString = "-exported-";
                    break;
                case SymDeferred:
                    csEntry.symTypeString = "-deferred-";
                    break;
                case SymSym:
                    csEntry.symTypeString = "SYM";
                    break;
#if API_VERSION_NUMBER >= 9
                case SymDia:
                    csEntry.symTypeString = "DIA";
                    break;
#endif
                case 8: //SymVirtual:
                    csEntry.symTypeString = "Virtual";
                    break;
                default:
                    //_snprintf( ty, sizeof ty, "symtype=%ld", (long) Module.SymType );
                    csEntry.symTypeString = NULL;
                    break;
                }

                // TODO: Mache dies sicher...!
                strcpy_s(csEntry.moduleName, Module.ModuleName);
                csEntry.baseOfImage = Module.BaseOfImage;
                strcpy_s(csEntry.loadedImageName, Module.LoadedImageName);
            } // got module info OK
            else
            {
                this->OnDbgHelpErr("SymGetModuleInfo64", GetLastError(), s.AddrPC.Offset);
            }
        } // we seem to have a valid PC

        CallstackEntryType et = nextEntry;
        if (frameNum == 0)
            et = firstEntry;
        this->OnCallstackEntry(et, csEntry);

        if (s.AddrReturn.Offset == 0)
        {
            this->OnCallstackEntry(lastEntry, csEntry);
            SetLastError(ERROR_SUCCESS);
            break;
        }
    } // for ( frameNum )

cleanup:
    if (pSym) free( pSym );

    if (context == NULL)
        ResumeThread(hThread);

    return TRUE;
}

BOOL __stdcall StackWalker::myReadProcMem(
    HANDLE      hProcess,
    DWORD64     qwBaseAddress,
    PVOID       lpBuffer,
    DWORD       nSize,
    LPDWORD     lpNumberOfBytesRead
    )
{
    if (s_readMemoryFunction == NULL)
    {
        SIZE_T st;
        BOOL bRet = ReadProcessMemory(hProcess, (LPVOID) qwBaseAddress, lpBuffer, nSize, &st);
        *lpNumberOfBytesRead = (DWORD) st;
        //printf("ReadMemory: hProcess: %p, baseAddr: %p, buffer: %p, size: %d, read: %d, result: %d\n", hProcess, (LPVOID) qwBaseAddress, lpBuffer, nSize, (DWORD) st, (DWORD) bRet);
        return bRet;
    }
    else
    {
        return s_readMemoryFunction(hProcess, qwBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead, s_readMemoryFunction_UserData);
    }
}

void StackWalker::OnLoadModule(LPCSTR img, LPCSTR mod, DWORD64 baseAddr, DWORD size, DWORD result, LPCSTR symType, LPCSTR pdbName, ULONGLONG fileVersion)
{
    CHAR buffer[STACKWALK_MAX_NAMELEN];
    if (fileVersion == 0)
        _snprintf_s(buffer, STACKWALK_MAX_NAMELEN, "%s:%s (%p), size: %d (result: %d), SymType: '%s', PDB: '%s'\n", img, mod, (LPVOID) baseAddr, size, result, symType, pdbName);
    else
    {
        DWORD v4 = (DWORD) fileVersion & 0xFFFF;
        DWORD v3 = (DWORD) (fileVersion>>16) & 0xFFFF;
        DWORD v2 = (DWORD) (fileVersion>>32) & 0xFFFF;
        DWORD v1 = (DWORD) (fileVersion>>48) & 0xFFFF;
        _snprintf_s(buffer, STACKWALK_MAX_NAMELEN, "%s:%s (%p), size: %d (result: %d), SymType: '%s', PDB: '%s', fileVersion: %d.%d.%d.%d\n", img, mod, (LPVOID) baseAddr, size, result, symType, pdbName, v1, v2, v3, v4);
    }
    OnOutput(buffer);
}

void StackWalker::OnCallstackEntry(CallstackEntryType eType, CallstackEntry &entry)
{
    CHAR buffer[STACKWALK_MAX_NAMELEN];
    if ( (eType != lastEntry) && (entry.offset != 0) )
    {
        if (entry.name[0] == 0)
            strcpy_s(entry.name, "(function-name not available)");
        if (entry.undName[0] != 0)
            strcpy_s(entry.name, entry.undName);
        if (entry.undFullName[0] != 0)
            strcpy_s(entry.name, entry.undFullName);
        if (entry.lineFileName[0] == 0)
        {
            strcpy_s(entry.lineFileName, "(filename not available)");
            if (entry.moduleName[0] == 0)
                strcpy_s(entry.moduleName, "(module-name not available)");
            _snprintf_s(buffer, STACKWALK_MAX_NAMELEN, "%p (%s): %s: %s\n", (LPVOID) entry.offset, entry.moduleName, entry.lineFileName, entry.name);
        }
        else
            _snprintf_s(buffer, STACKWALK_MAX_NAMELEN, "%s (%d): %s\n", entry.lineFileName, entry.lineNumber, entry.name);
        OnOutput(buffer);
    }
}

void StackWalker::OnDbgHelpErr(LPCSTR szFuncName, DWORD gle, DWORD64 addr)
{
    CHAR buffer[STACKWALK_MAX_NAMELEN];
    _snprintf_s(buffer, STACKWALK_MAX_NAMELEN, "ERROR: %s, GetLastError: %d (Address: %p)\n", szFuncName, gle, (LPVOID) addr);
    OnOutput(buffer);
}

void StackWalker::OnSymInit(LPCSTR szSearchPath, DWORD symOptions, LPCSTR szUserName)
{
    CHAR buffer[STACKWALK_MAX_NAMELEN];
    _snprintf_s(buffer, STACKWALK_MAX_NAMELEN, "SymInit: Symbol-SearchPath: '%s', symOptions: %d, UserName: '%s'\n", szSearchPath, symOptions, szUserName);
    OnOutput(buffer);
    // Also display the OS-version
#if _MSC_VER <= 1200
    OSVERSIONINFOA ver;
    ZeroMemory(&ver, sizeof(OSVERSIONINFOA));
    ver.dwOSVersionInfoSize = sizeof(ver);
    if (GetVersionExA(&ver) != FALSE)
    {
        _snprintf_s(buffer, STACKWALK_MAX_NAMELEN, "OS-Version: %d.%d.%d (%s)\n", 
            ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber,
            ver.szCSDVersion);
        OnOutput(buffer);
    }
#else
    OSVERSIONINFOEXA ver;
    ZeroMemory(&ver, sizeof(OSVERSIONINFOEXA));
    ver.dwOSVersionInfoSize = sizeof(ver);
    if (GetVersionExA( (OSVERSIONINFOA*) &ver) != FALSE)
    {
        _snprintf_s(buffer, STACKWALK_MAX_NAMELEN, "OS-Version: %d.%d.%d (%s) 0x%x-0x%x\n", 
            ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber,
            ver.szCSDVersion, ver.wSuiteMask, ver.wProductType);
        OnOutput(buffer);
    }
#endif
}

void StackWalker::OnOutput(LPCSTR buffer)
{
    OutputDebugStringA(buffer);
}

#endif

