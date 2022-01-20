
#pragma once

#include <array>
#include <string>
#include <syslog.h>

#include "lldefs.h"

namespace ll{

class Syslog{
  public:
    using SeverityMap = std::array<int, levels>;

    inline Syslog(
        const char *ident, 
        int option = LOG_CONS | LOG_PID | LOG_NDELAY, 
        int facility = LOG_USER,
        const SeverityMap& severityMap = defaultSeverityMap()
    );
    inline Syslog(const Syslog&) = delete;
    inline ~Syslog();

    inline void log(const std::string& str, level lev) const;

  private:
    const SeverityMap& severityMap_;
    inline static const SeverityMap& defaultSeverityMap();
  
};

Syslog::Syslog (const char* ident, 
                int option, int facility, 
                const SeverityMap& severityMap):
                severityMap_(severityMap){
    openlog(ident, option, facility);
}

Syslog::~Syslog(){
    closelog();
}

void Syslog::log(const std::string& str, level lev) const{
    syslog(severityMap_[lev], "%s", str.c_str());
}

const Syslog::SeverityMap& Syslog::defaultSeverityMap(){
    static SeverityMap ret{
        LOG_CRIT,
        LOG_ERR,
        LOG_WARNING,
        LOG_NOTICE,
        LOG_INFO,
        LOG_DEBUG
    };

    return ret;
}

}