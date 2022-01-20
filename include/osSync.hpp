
#pragma once

#include <iostream>
#include <string>

namespace ll{

class OStreamSync{
  public:
    inline OStreamSync(std::ostream& os);

    inline void log(const std::string& str);

  private:
    std::ostream& os_;
    OStreamSync(const OStreamSync&) = delete;
};

OStreamSync::OStreamSync(std::ostream& os): os_(os){
}

void OStreamSync::log(const std::string& str){
    os_ << str << std::endl;
}

}