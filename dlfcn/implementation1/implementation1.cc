#include <iostream>

#include "implementation1.hpp"

Implementation1::Implementation1() {
  std::cout << "Implementation1() is called" << std::endl;
}

void Implementation1::exec() {
  std::cout << "Implementation1::exec() is called" << std::endl;
}

Implementation1::~Implementation1() {
  std::cout << "~Implementation1() is called" << std::endl;
}

extern "C" {
  Implementation1* create() {
    return new Implementation1();
  }
}
