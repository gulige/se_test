#include "iexp.h"
#include <float.h>
#include <string>

using namespace std;

#if WIN32

static _se_translator_function		ls_PreSeTransFunction;
static _invalid_parameter_handler	ls_PreParamHandler;
static _purecall_handler			ls_PrePurecallHandler;
static bool IsFloatingPointException(EXCEPTION_POINTERS* pExp)
{
    UINT32 dwExceptionCode = pExp->ExceptionRecord->ExceptionCode;
    switch (dwExceptionCode)
    {
    case EXCEPTION_FLT_DENORMAL_OPERAND:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INEXACT_RESULT:
    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_FLT_OVERFLOW:
    case EXCEPTION_FLT_UNDERFLOW:
    case STATUS_FLOAT_MULTIPLE_FAULTS:
    case STATUS_FLOAT_MULTIPLE_TRAPS:
        return true;

    default:
        return false;
    }
}

void Trans_Func( unsigned int u, EXCEPTION_POINTERS* pExp )
{
    //����������ͰѶ�ջд�������׳�ȥ���ò����ֳ���
    ExpStackWalker sw;

    if (IsFloatingPointException(pExp))
    {
        //��������������쳣�ı�־λ����Ȼ�´��쳣�ֻᱬ��
        _clearfp();

        sw.OnHandleSEException(pExp, true);
    }
    else
    {
        sw.OnHandleSEException(pExp, false);
    }

    throw Own_Exception();
}

void SetControlFP()
{
    unsigned control_word;
    _controlfp_s(&control_word,_PC_53,_MCW_PC);
    _controlfp_s(&control_word,0,0);
    unsigned cw = control_word;
    cw &=~(_EM_ZERODIVIDE|_EM_DENORMAL|_EM_OVERFLOW);
    _controlfp_s(&control_word,cw,_MCW_EM);
    SetErrorMode( 0 );
}

void Set_SE_Translator()
{
    _set_se_translator(Trans_Func);
}

static void InvalidParameterHandler(const wchar_t* , const wchar_t* , const wchar_t* , unsigned int , uintptr_t )
{
    ExpStackWalker sw;
    sw.OnHandleInvalidParameter();

    throw Own_Exception();
}

static void PurecallHandler()
{
    ExpStackWalker sw;
    sw.OnHandlePurecall();

    throw Own_Exception();
}

#endif //Win32

bool BeInDebugging()
{
#if WIN32
    
    return IsDebuggerPresent();

#else 

    return false;

#endif
}

void InitExpHandler()
{
#if WIN32
    
    //���ø����쳣��־λ
    //SetControlFP();

    //����windows�ṹ���쳣������
    ls_PreSeTransFunction   = _set_se_translator(Trans_Func);       

    //����C����ʱ����������������
    ls_PreParamHandler      = _set_invalid_parameter_handler(InvalidParameterHandler);

    //���ô��麯�����ʴ���
    ls_PrePurecallHandler   = _set_purecall_handler(PurecallHandler);

#else

#endif
}

void UnitExpHandler()
{
#if WIN32
    _set_purecall_handler(ls_PrePurecallHandler);
    _set_se_translator(ls_PreSeTransFunction);
    _set_invalid_parameter_handler(ls_PreParamHandler);
#else

#endif
}

