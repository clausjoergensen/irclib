// This code is licensed under MIT license (see LICENSE.txt for details)
#pragma once

#include <string>

namespace LibIrc {

class IrcMessageSource {
  public:
    virtual std::string getName() = 0;
};

} // namespace LibIrc
