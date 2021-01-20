#ifndef INTERFACE_HPP_
#define INTERFACE_HPP_

#include <iostream>

class Interface {
 public:
  Interface() {
    std::cout << "Interface() is called" << std::endl;
  }

  virtual void exec() {}
  virtual ~Interface() {
    std::cout << "~Interface() is called" << std::endl;
  }
};

#endif
