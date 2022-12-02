#include "expstackwalker.h"

#ifdef WIN32

#include <sstream>
#include <tchar.h>

#ifdef _DEBUG
#define EXP_PRINTF(...) printf(__VA_ARGS__)
#else
#define EXP_PRINTF(...) printf("程序发生异常，请查看log ！ \n");
#endif

ExpStackWalker::ExpStackWalker()
{
}

ExpStackWalker::~ExpStackWalker()
{

}

void ExpStackWalker::OnOutput(LPCSTR szText) 
{ 
    m_strExpMsg.append(szText);
}

void ExpStackWalker::OnHandleCppException(std::exception& std_exp)
{
    OnOutput("\n发生C++异常: ");
    OnOutput(std_exp.what());
    OnOutput("\n");
    ShowCallstack();
    OnOutput("\n");

    EXP_PRINTF(m_strExpMsg.c_str());
    //EXP_INFO(m_strExpMsg.c_str());

    ExpStackWalkerMgr::Inst()->setLastExpInfo(m_strExpMsg);
}

void ExpStackWalker::OnHandleSEException(EXCEPTION_POINTERS* pExp, bool bFloatExp)
{
    if (bFloatExp)
        OnOutput("\n发生浮点异常! \n");
    else
    {
        ostringstream os;
        os<<"发生Windows结构化异常! 异常号:  "<<ExpStackWalkerMgr::Inst()->GetExpNameById(pExp->ExceptionRecord->ExceptionCode).c_str()<<endl;
        OnOutput(os.str().c_str());
    }
    ShowCallstack(GetCurrentThread(), pExp->ContextRecord);
    OnOutput("\n");

    EXP_PRINTF(m_strExpMsg.c_str());
    //EXP_INFO(m_strExpMsg.c_str());

    ExpStackWalkerMgr::Inst()->setLastExpInfo(m_strExpMsg);
}

void ExpStackWalker::OnHandleInvalidParameter()
{
    OnOutput("\nC运行时参数错误！ \n");
    ShowCallstack();
    OnOutput("\n");

    EXP_PRINTF(m_strExpMsg.c_str());
    //EXP_INFO(m_strExpMsg.c_str());

    ExpStackWalkerMgr::Inst()->setLastExpInfo(m_strExpMsg);
}

void ExpStackWalker::OnHandlePurecall()
{
    OnOutput("\n纯虚函数访问错误! \n");
    ShowCallstack();
    OnOutput("\n");

    EXP_PRINTF(m_strExpMsg.c_str());
    //EXP_INFO(m_strExpMsg.c_str());

    ExpStackWalkerMgr::Inst()->setLastExpInfo(m_strExpMsg);
}

//---------------------------------- ExpStackWalkerMgr ---------------------------------------------------

ExpStackWalkerMgr::ExpStackWalkerMgr()
{
    m_mapExpType.insert(make_pair(EXCEPTION_ACCESS_VIOLATION, "EXCEPTION_ACCESS_VIOLATION"));
    m_mapExpType.insert(make_pair(EXCEPTION_DATATYPE_MISALIGNMENT, "EXCEPTION_DATATYPE_MISALIGNMENT"));
    m_mapExpType.insert(make_pair(EXCEPTION_BREAKPOINT, "EXCEPTION_BREAKPOINT"));
    m_mapExpType.insert(make_pair(EXCEPTION_SINGLE_STEP, "EXCEPTION_SINGLE_STEP"));
    m_mapExpType.insert(make_pair(EXCEPTION_ARRAY_BOUNDS_EXCEEDED, "EXCEPTION_ARRAY_BOUNDS_EXCEEDED"));
    m_mapExpType.insert(make_pair(EXCEPTION_FLT_DENORMAL_OPERAND, "EXCEPTION_FLT_DENORMAL_OPERAND"));
    m_mapExpType.insert(make_pair(EXCEPTION_FLT_DIVIDE_BY_ZERO, "EXCEPTION_FLT_DIVIDE_BY_ZERO"));
    m_mapExpType.insert(make_pair(EXCEPTION_FLT_INEXACT_RESULT, "EXCEPTION_FLT_INEXACT_RESULT"));
    m_mapExpType.insert(make_pair(EXCEPTION_FLT_INVALID_OPERATION, "EXCEPTION_FLT_INVALID_OPERATION"));
    m_mapExpType.insert(make_pair(EXCEPTION_FLT_OVERFLOW, "EXCEPTION_FLT_OVERFLOW"));
    m_mapExpType.insert(make_pair(EXCEPTION_FLT_STACK_CHECK, "EXCEPTION_FLT_STACK_CHECK"));
    m_mapExpType.insert(make_pair(EXCEPTION_FLT_UNDERFLOW, "EXCEPTION_FLT_UNDERFLOW"));
    m_mapExpType.insert(make_pair(EXCEPTION_INT_DIVIDE_BY_ZERO, "EXCEPTION_INT_DIVIDE_BY_ZERO"));
    m_mapExpType.insert(make_pair(EXCEPTION_INT_OVERFLOW, "EXCEPTION_INT_OVERFLOW"));
    m_mapExpType.insert(make_pair(EXCEPTION_PRIV_INSTRUCTION, "EXCEPTION_PRIV_INSTRUCTION"));
    m_mapExpType.insert(make_pair(EXCEPTION_IN_PAGE_ERROR, "EXCEPTION_IN_PAGE_ERROR"));
    m_mapExpType.insert(make_pair(EXCEPTION_ILLEGAL_INSTRUCTION, "EXCEPTION_ILLEGAL_INSTRUCTION"));
    m_mapExpType.insert(make_pair(EXCEPTION_NONCONTINUABLE_EXCEPTION, "EXCEPTION_NONCONTINUABLE_EXCEPTION"));
    m_mapExpType.insert(make_pair(EXCEPTION_STACK_OVERFLOW, "EXCEPTION_STACK_OVERFLOW"));
    m_mapExpType.insert(make_pair(EXCEPTION_INVALID_DISPOSITION, "EXCEPTION_INVALID_DISPOSITION"));
    m_mapExpType.insert(make_pair(EXCEPTION_GUARD_PAGE, "EXCEPTION_GUARD_PAGE"));
    m_mapExpType.insert(make_pair(EXCEPTION_INVALID_HANDLE, "EXCEPTION_INVALID_HANDLE"));
    //m_mapExpType.insert(make_pair(EXCEPTION_POSSIBLE_DEADLOCK, "EXCEPTION_POSSIBLE_DEADLOCK"));
}

ExpStackWalkerMgr::~ExpStackWalkerMgr()
{

}

ExpStackWalkerMgr* ExpStackWalkerMgr::Inst()
{
    static ExpStackWalkerMgr ms_ExpWalkerMgr;
    return &ms_ExpWalkerMgr;
}

void ExpStackWalkerMgr::setLastExpInfo(string strExpInfo)
{
    m_strLastErrInfo = strExpInfo;
}

string& ExpStackWalkerMgr::GetLastExpInfo()
{
    return m_strLastErrInfo;
}

string ExpStackWalkerMgr::GetExpNameById(UINT32 dwExpCode)
{
    MapExpTye::iterator it = m_mapExpType.find(dwExpCode);
    if (it != m_mapExpType.end())
    {
        return it->second;
    }
    return "未知类型异常！";
}

#endif





