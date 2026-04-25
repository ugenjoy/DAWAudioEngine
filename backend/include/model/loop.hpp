// backend/include/model/loop.hpp
#pragma once

#include <string>

struct Loop {
  std::string id;
  double start{0.0};  // seconds
  double end{0.0};    // seconds
};
