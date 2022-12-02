#ifndef __I_EXP__
#define __I_EXP__

#define MAX_STACK_WALKER_STEP 15

void InitExpHandler();
void UnitExpHandler();
bool BeInDebugging();      //?§Ø??????????????????



#ifdef WIN32   //win?·Ú???

#include "expstackwalker.h"
#include "ownexception.h"

#define Fx_Try \
    try \
    { \
        try \
        {

#define Fx_Catch \
        } \
        catch(std::exception& std_exp) \
        { \
            ExpStackWalker sw; \
            sw.OnHandleCppException(std_exp); \
            throw Own_Exception(); \
        } \
        catch(Own_Exception& se_exp) \
        { \
            throw Own_Exception(); \
        } \
    } \
    catch(Own_Exception& exp) \
    {

#define Fx_Try_End \
    }

#else

#define Fx_Try \
    {

#define Fx_Catch \
    } \
    if (false) \
    {


#define Fx_Try_End \
    }

#endif

#endif //__I_EXP__
