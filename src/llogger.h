/*
  Copyright (C) 2020-2021 SdtElectronics <null@std.uestc.edu.cn>
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

#include <array>
#include <chrono>
#include <ctime>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

class llogger;

class llfmt{
  public:
    enum info_t: char{level = 1, time};
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

#if ((defined(_MSVC_LANG) && _MSVC_LANG < 201703L) || __cplusplus < 201703L)
// Required before C++17
const std::array<const char*, 5> llfmt::defLevelNames;
#endif

std::vector<size_t>& llfmt::fmtOpt(){
    return *(fmtOrdIter++);
}

void llfmt::resetIters(){
    fmtStrIter = fmtStrs.begin();
    fmtOrdIter = fmtOrds.begin();
    fmtLmbIter = fmtLmbs.begin();
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


class llogger{
  public:
    enum level: signed char{silent = -1, fatal, error, warning, notice, info};
    enum fmtStr_t: char{fmtStr};

    llogger(std::ostream& os, level lev, llfmt& format = *defaultFmt);

  private:
    struct logger{
        logger(llogger& holder);
        ~logger();
        template <typename T>
        void opImpl(const T& content, std::true_type tp);
        template <typename T>
        void opImpl(const T& content, std::false_type tp);

        inline void putFmtStr();

        llogger& holder_;

        template <typename T, typename = void>
        struct is_callable: public std::false_type {};

        template <typename T>
        #if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
        struct is_callable<
            T,
            std::enable_if_t<!std::is_same_v<void, std::invoke_result_t<T> > >
        >: public std::true_type {};
        #else
        struct is_callable<
            T,
            std::enable_if<!std::is_same<void, typename std::result_of<T>::type>::value>
        >: public std::true_type {};
        #endif

    };

    template <typename T>
    friend llogger::logger&& operator << (llogger::logger&& wrap, const T& content);
    friend llogger::logger&& operator << (llogger::logger&& wrap, llogger::fmtStr_t);

    inline void putFmtStr();
    inline void timeStamp();
    inline void putLogLev();
    inline void putFmtLmb();

    const std::array<void (llogger::*)(), 4> fmtCbs{
        &llogger::putFmtStr, 
        &llogger::putLogLev, 
        &llogger::timeStamp, 
        &llogger::putFmtLmb
    };

    bool enable_;
    llfmt& fmt;
    std::ostream& os_;
    std::ostringstream buf_;
    level level_;
    level curLev;

    static std::shared_ptr<llfmt> defaultFmt;

  public:
    inline logger operator() ();
    inline logger operator() (llogger::level lev);
    inline logger operator() (bool predicate);
    inline logger operator() (llogger::level lev, bool predicate);

    template<typename T = std::chrono::microseconds>
    static inline long long tElapsed(const std::chrono::steady_clock::time_point& start);
};

std::shared_ptr<llfmt> llogger::defaultFmt = []{
    auto llp = std::make_shared<llfmt>();
    *llp << "[" << llfmt::time  << "] "
                << llfmt::level << ": "
                << llfmt::logStr;
    return llp;
}();

llogger::llogger(std::ostream& os, level lev, llfmt& format): os_(os), 
                                                             level_(lev),
                                                             curLev(llogger::info),
                                                             fmt(format){
};


llogger::logger::logger(llogger& holder): holder_(holder){
    llogger::logger::putFmtStr();
}

llogger::logger::~logger(){
    if(holder_.enable_){
        holder_.os_ << holder_.buf_.str() << std::endl;
        std::ostringstream().swap(holder_.buf_);
    }
}

void llogger::putFmtStr(){
    buf_ << *(fmt.fmtStrIter++);
}

void llogger::timeStamp(){
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    buf_ << std::put_time(std::localtime(&now_time), " %Y-%m-%d %X ");
}

void llogger::putLogLev(){
    buf_ << fmt._levelNames[static_cast<size_t>(curLev)];
}

void llogger::putFmtLmb(){
    buf_ << (*(fmt.fmtLmbIter++))();
}

llogger::logger llogger::operator() (){
    fmt.resetIters();
    enable_ = curLev <= level_;
    return logger(*this);
}

llogger::logger llogger::operator() (llogger::level lev){
    fmt.resetIters();
    curLev = lev;
    enable_ = curLev <= level_;
    return logger(*this);
}

llogger::logger llogger::operator() (bool predicate){
    fmt.resetIters();
    enable_ = curLev <= level_ && predicate;
    return logger(*this);
}

llogger::logger llogger::operator() (llogger::level lev, bool predicate){
    fmt.resetIters();
    curLev = lev;
    enable_ = curLev <= level_ && predicate;
    return logger(*this);
}

template<typename T>
long long llogger::tElapsed(const std::chrono::steady_clock::time_point& start){
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<T>(end - start).count();
}

void llogger::logger::putFmtStr(){
    if(holder_.enable_){
        for(size_t i: holder_.fmt.fmtOpt()){
            (holder_.*(holder_.fmtCbs[i]))();
        }
    }
}

template <typename T>
void llogger::logger::opImpl(const T& content, std::false_type tp){
    holder_.buf_ << content;
}

template <typename T>
void llogger::logger::opImpl(const T& content, std::true_type tp){
    holder_.buf_ << content();
}

llogger::logger&& operator << (llogger::logger&& wrap, llogger::fmtStr_t){
    wrap.putFmtStr();
    return std::move(wrap);
}

template <typename T>
llogger::logger&& operator << (llogger::logger&& wrap, const T& content){
    if(wrap.holder_.enable_){
        wrap.opImpl(content, llogger::logger::is_callable<T>{});
    }
    return std::move(wrap);
}