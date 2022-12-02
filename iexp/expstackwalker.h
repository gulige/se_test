#ifndef __EXP_STACK_WALKER__
#define __EXP_STACK_WALKER__

#ifdef WIN32

#include "stackwalker.h"
#include <string>
#include <map>

using namespace std;

class ExpStackWalker
    : public StackWalker
{
public:
    ExpStackWalker();
    virtual ~ExpStackWalker();

    virtual void OnSymInit(LPCSTR szSearchPath, DWORD symOptions, LPCSTR szUserName){}
    virtual void OnLoadModule(LPCSTR img, LPCSTR mod, DWORD64 baseAddr, DWORD size, DWORD result, LPCSTR symType, LPCSTR pdbName, ULONGLONG fileVersion){}
    // virtual void OnCallstackEntry(CallstackEntryType eType, CallstackEntry &entry){}
    virtual void OnDbgHelpErr(LPCSTR szFuncName, DWORD gle, DWORD64 addr){}

    //暂时只有到这一个
    virtual void OnOutput(LPCSTR szText);

    void OnHandleCppException(std::exception& std_exp);
    void OnHandleSEException(EXCEPTION_POINTERS* pExp, bool bFloatExp);
    void OnHandleInvalidParameter();
    void OnHandlePurecall();

private:
    string                  m_strExpMsg;
};

class ExpStackWalkerMgr
{
    typedef map<UINT32, string> MapExpTye;
public:
    ExpStackWalkerMgr();
    ~ExpStackWalkerMgr();

    static ExpStackWalkerMgr* Inst();

    void setLastExpInfo(string strExpInfo);
    string& GetLastExpInfo();

    string GetExpNameById(UINT32 dwExpCode);
private:
    MapExpTye               m_mapExpType;
    string                  m_strLastErrInfo;
};


#endif

#endif

