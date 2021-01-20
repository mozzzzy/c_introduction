#include <iostream>

#include "../interface.hpp"

class Implementation1 : public Interface {
 public:
  Implementation1();
  void exec();
  ~Implementation1();
};
