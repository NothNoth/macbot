#include "Exception.hpp"

Exception::Exception(string msg)
{
  this->_msg = msg;
}

Exception::~Exception()
{
}

string Exception::GetMessage()
{
  return _msg;
}
