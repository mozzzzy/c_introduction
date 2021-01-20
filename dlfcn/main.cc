#include <dlfcn.h>
#include <iostream>

#include "interface.hpp"

int main() {
  void* handle;
  handle = dlopen("implementation1/libimplementation1.so", RTLD_LAZY);
  if (!handle) {
    std::cerr << dlerror() << std::endl;
    return EXIT_FAILURE;
  }

  Interface* (*creator)();
  creator = (Interface* (*)()) dlsym(handle, "create");
  Interface* impl = creator();

  impl->exec();

  delete(impl);

  return EXIT_SUCCESS;
}
