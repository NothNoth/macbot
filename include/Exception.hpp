#include <iostream>
using namespace std;

class Exception
{
  public:
    Exception(string msg);
    virtual ~Exception();
    string GetMessage();
  private:
    string _msg;
};
