/*
  Copyright (C) 2020 SdtElectronics <null@std.uestc.edu.cn>
  All rights reserved.
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
  * Neither the name of the copyright holder nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY  COPYRIGHT HOLDERS AND CONTRIBUTORS ''AS IS'' 
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
  ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <string>
#include <sstream>

#include "llogger.h"

thread_local llogger* llogger::curlogger = nullptr;

std::shared_ptr<llfmt> llogger::defaultFmt = []{
    auto llp = std::make_shared<llfmt>();
    *llp << "[" << llfmt::time  << "] "
                << llfmt::level << ": "
                << llfmt::logStr;
    return llp;
}();

llogger::llogger(std::ostream& os, level lev, llfmt& format): _os(os), 
                                                             _level(lev),
                                                             curLev(llogger::info),
                                                             fmt(format){
    llogger::curlogger = this;
};

void llogger::activate(){
    llogger::curlogger = this;
}

llogger::logger::logger(){
    llogger::logger::putFmtStr();
}

llogger::logger::~logger(){
    //llogger::curlogger->log();
    if(llogger::curlogger->enable){
        llogger::curlogger->_os << std::endl;
    }
}

llfmt::llfmt(const std::array<const char*, 5>& levelNames): _levelNames(levelNames),
                                                            fmtOrds(1),
                                                            fmtStrIter(fmtStrs.begin()),
                                                            fmtOrdIter(fmtOrds.begin()),
                                                            fmtLmbIter(fmtLmbs.begin()){
}

llfmt& llfmt::operator << (llfmt::info_t info){
    fmtOrds.back().push_back(static_cast<size_t>(info));
    return *this;
}

llfmt& llfmt::operator << (const char* fmtStr){
    fmtOrds.back().push_back(0U);
    fmtStrs.push_back(std::string(fmtStr));
    return *this;
}

llfmt& llfmt::operator << (const std::string& fmtStr){
    fmtOrds.back().push_back(0U);
    fmtStrs.push_back(std::string(fmtStr));
    return *this;
}

llfmt& llfmt::operator << (llfmt::logStr_t){
    fmtOrds.push_back(std::vector<size_t>());
    return *this;
}

llfmt& llfmt::operator << (const fmtCallback& fmtLmb){
    fmtOrds.back().push_back(3U);
    fmtLmbs.push_back(fmtLmb);
    return *this;
}