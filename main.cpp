#include <QCoreApplication>

#include <iostream>
using namespace std;

#include "iexp/iexp.h"

void test_throw();
void test_null_pointer();
void test_div_zero();
void test_out_of_bounds();
void test_out_of_bounds2();
void test_out_of_bounds3();

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if (!BeInDebugging())
    {
        InitExpHandler();
    }

    Fx_Try{
        test_throw();
    }
    Fx_Catch{}
    Fx_Try_End

    Fx_Try
    {
        test_null_pointer();
    }
    Fx_Catch{}
    Fx_Try_End

    Fx_Try
    {
        test_div_zero();
    }
    Fx_Catch{}
    Fx_Try_End

    Fx_Try
    {
        test_out_of_bounds();
    }
    Fx_Catch{}
    Fx_Try_End

    Fx_Try
    {
        test_out_of_bounds2();
    }
    Fx_Catch{}
    Fx_Try_End

    Fx_Try
    {
        test_out_of_bounds3();
    }
    Fx_Catch{}
    Fx_Try_End

    if (!BeInDebugging())
    {
        UnitExpHandler();
    }

    cout << endl << "still running..." << endl;

    return a.exec();
}

void test_throw()
{
    //try {
        throw std::exception("an exception");
    //} catch(std::exception e) {
    //    cout << "throw " << e.what() << endl;
    //}
}

void test_null_pointer()
{
    //try {
        int* p = nullptr;
        *p = 10;
    //} catch(...) {
    //    cout << "null pointer" << endl;
    //}
}

void test_div_zero()
{
    //try {
        int a = 0;
        float b = 10 / a;
        float c = b + 1;
    //} catch(...) {
    //    cout << "div zero" << endl;
    //}
}

void test_out_of_bounds()
{
    int* p = new int[3];
    //try {
        *(p + 1100) = 0;
    //} catch(...) {
    //    cout << "out of bounds" << endl;
    //}
}

void test_out_of_bounds2()
{
    int arr[3];
    arr[1024] = 0;
}

struct A
{
    char arr_a[3];
    int arr_b[1024];
};

void test_out_of_bounds3()
{
    A* p = new A;
    p->arr_a[12288] = 0;
}

