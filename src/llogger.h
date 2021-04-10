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

#pragma once

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <memory>

class llogger;

class llfmt{
  public:
    enum info_t: char{time = 1, level};
    enum logStr_t: char{logStr};
    using fmtCallback = std::function<std::string(void)>;

    llfmt(const std::array<const char *, 5> &levelNames = defLevelNames);

    inline std::vector<size_t>& fmtOpt();
    inline void resetIters();

    llfmt& operator << (llfmt::info_t info);
    llfmt& operator << (llfmt::logStr_t);
    llfmt& operator << (const char* fmtStr);
    llfmt& operator << (const std::string& fmtStr);
    llfmt& operator << (const fmtCallback& fmtLmb);

  private:
    friend llogger;
    std::vector<std::vector<size_t> > fmtOrds;
    std::vector<std::string>          fmtStrs;
    std::vector<fmtCallback>          fmtLmbs;

    std::vector<std::string>::iterator          fmtStrIter;
    std::vector<std::vector<size_t> >::iterator fmtOrdIter;
    std::vector<fmtCallback>::iterator          fmtLmbIter;


    const std::array<const char*, 5> _levelNames;

    static constexpr std::array<const char*, 5> defLevelNames{
        "\033[1m\033[31m FATAL \033[0m", 
        "\033[1m\033[31m ERROR \033[0m", 
        "\033[1m\033[35mWARNING\033[0m", 
        "\033[1m\033[34mNOTICE \033[0m", 
        "\033[1m\033[36m INFO  \033[0m"
    };
};

std::vector<size_t>& llfmt::fmtOpt(){
    return *(fmtOrdIter++);
}

void llfmt::resetIters(){
    fmtStrIter = fmtStrs.begin();
    fmtOrdIter = fmtOrds.begin();
    fmtLmbIter = fmtLmbs.begin();
}

class llogger{
  public:
    enum level: signed char{silent = -1, fatal, error, warning, notice, info};
    enum fmtStr_t: char{fmtStr};

    llogger(std::ostream& os, level lev, llfmt& format = *defaultFmt);

    inline void activate();

  private:
    struct logger{
        logger();
        ~logger();
        template <typename T>
        void opImpl(const T& content, std::true_type tp);
        template <typename T>
        void opImpl(const T& content, std::false_type tp);

        static inline void putFmtStr();

        template <typename T, typename = void>
        struct is_callable : public std::false_type {};

        template <typename T>
        struct is_callable<
            T,
            std::enable_if_t<!std::is_same_v<void, std::invoke_result_t<T> > >
        >: public std::true_type {};
    };

    template <typename T>
    friend llogger::logger&& operator << (llogger::logger&& wrap, const T& content);
    friend llogger::logger&& operator << (llogger::logger&& wrap, llogger::fmtStr_t);

    inline void putFmtStr();
    inline void timeStamp();
    inline void putLogLev();
    inline void putFmtLmb();

    const std::array<void (llogger::*)(), 4> fmtCbs {
        &llogger::putFmtStr, &llogger::timeStamp, &llogger::putLogLev, &llogger::putFmtLmb
    };

    bool enable;
    llfmt& fmt;
    std::ostream& _os;
    level _level;
    level curLev;
    thread_local static llogger* curlogger;

    static std::shared_ptr<llfmt> defaultFmt;

  public:
    inline logger operator() ();
    inline logger operator() (llogger::level lev);
    inline logger operator() (bool predicate);
    inline logger operator() (llogger::level lev, bool predicate);
};

void llogger::putFmtStr(){
    _os << *(fmt.fmtStrIter++);
}

void llogger::timeStamp(){
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    _os << std::put_time(std::localtime(&now_time), " %Y-%m-%d %X ");
}

void llogger::putLogLev(){
    _os << fmt._levelNames[static_cast<size_t>(curLev)];
}

void llogger::putFmtLmb(){
    _os << (*(fmt.fmtLmbIter++))();
}

llogger::logger llogger::operator() (){
    llogger& curlogger = *llogger::curlogger;
    curlogger.fmt.resetIters();
    curlogger.enable = curlogger.curLev <= curlogger._level;
    return logger();
}

llogger::logger llogger::operator() (llogger::level lev){
    llogger& curlogger = *llogger::curlogger;
    curlogger.fmt.resetIters();
    curlogger.curLev = lev;
    curlogger.enable = curlogger.curLev <= curlogger._level;
    return logger();
}

llogger::logger llogger::operator() (bool predicate){
    llogger& curlogger = *llogger::curlogger;
    curlogger.fmt.resetIters();
    curlogger.enable = curlogger.curLev <= curlogger._level && predicate;
    return logger();
}

llogger::logger llogger::operator() (llogger::level lev, bool predicate){
    llogger& curlogger = *llogger::curlogger;
    curlogger.fmt.resetIters();
    curlogger.curLev = lev;
    curlogger.enable = curlogger.curLev <= curlogger._level && predicate;
    return logger();
}

void llogger::logger::putFmtStr(){
    if(llogger::curlogger->enable){
        for(size_t i: llogger::curlogger->fmt.fmtOpt()){
            (llogger::curlogger->*(llogger::curlogger->fmtCbs[i]))();
        }
    }
}

template <typename T>
void llogger::logger::opImpl(const T& content, std::false_type tp){
    llogger::curlogger->_os << content;
}

template <typename T>
void llogger::logger::opImpl(const T& content, std::true_type tp){
    llogger::curlogger->_os << content();
}

llogger::logger&& operator << (llogger::logger&& wrap, llogger::fmtStr_t){
    llogger::logger::putFmtStr();
    return ::std::move(wrap);
}

template <typename T>
llogger::logger&& operator << (llogger::logger&& wrap, const T& content){
    if(llogger::curlogger->enable){
        wrap.opImpl(content, llogger::logger::is_callable<T>{});
    }
    return ::std::move(wrap);
}