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

// Polyfill of C++17 features
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
    template<typename F, typename ...Args>
    using invokeResult = std::invoke_result_t<F, Args...>;

    template<typename F, typename ...Args>
    using isCallable = std::is_invocable<F, Args...>;
#else
    #include "invoke.hpp"
    template<typename F, typename ...Args>
    using invokeResult = invoke_hpp::invoke_result_t<F, Args...>;

    template<typename F, typename ...Args>
    using isCallable = invoke_hpp::is_invocable<F, Args...>;
#endif

class llogger;

struct fmtItrs{
    using fmtCallback = std::function<std::string(void)>;
    std::vector<std::string>::const_iterator          fmtStrIter;
    std::vector<std::vector<size_t> >::const_iterator fmtOrdIter;
    std::vector<fmtCallback>::const_iterator          fmtLmbIter;
};

class llfmt{
  public:
    enum infoType: char{level = 1, time};
    enum dataType: char{logStr};
    using fmtCallback = std::function<std::string(void)>;

    inline llfmt(const std::array<const char *, 5> &levelNames = defaultLevelStr());

    inline const std::vector<size_t>& fmtOpt(fmtItrs& state) const;
    inline fmtItrs getIters() const;

    inline llfmt& operator << (llfmt::infoType info);
    inline llfmt& operator << (llfmt::dataType);
    inline llfmt& operator << (const char* fmtStr);
    inline llfmt& operator << (const std::string& fmtStr);
    inline llfmt& operator << (const fmtCallback& fmtLmb);

  private:
    friend class llogger;
    std::vector<std::vector<size_t> > fmtOrds;
    std::vector<std::string>          fmtStrs;
    std::vector<fmtCallback>          fmtLmbs;

    const std::array<const char*, 5>& levelNames_;

    static const std::array<const char*, 5>& defaultLevelStr();
};

const std::vector<size_t>& llfmt::fmtOpt(fmtItrs& state) const{
    return *(state.fmtOrdIter++);
}

fmtItrs llfmt::getIters() const{
    return {
        fmtStrs.cbegin(),
        fmtOrds.cbegin(),
        fmtLmbs.cbegin()
    };
}

llfmt::llfmt(const std::array<const char*, 5>& levelNames): levelNames_(levelNames),
                                                            fmtOrds(1){
}

llfmt& llfmt::operator << (llfmt::infoType info){
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

llfmt& llfmt::operator << (llfmt::dataType){
    fmtOrds.push_back(std::vector<size_t>());
    return *this;
}

llfmt& llfmt::operator << (const fmtCallback& fmtLmb){
    fmtOrds.back().push_back(3U);
    fmtLmbs.push_back(fmtLmb);
    return *this;
}

const std::array<const char*, 5>& llfmt::defaultLevelStr(){
    static constexpr std::array<const char*, 5> ret{
        "\033[1m\033[31m FATAL \033[0m", 
        "\033[1m\033[31m ERROR \033[0m", 
        "\033[1m\033[35mWARNING\033[0m", 
        "\033[1m\033[34mNOTICE \033[0m", 
        "\033[1m\033[36m INFO  \033[0m"
    };

    return ret;
};


class llogger{
  public:
    enum level: signed char{silent = -1, fatal, error, warning, notice, info};
    enum fmtStrType: char{fmtStr};

    llogger(std::ostream& os, level lev, const llfmt& format = defaultFmt());
    llogger(const llogger& other);

  private:
    struct logger{
        inline logger(llogger& holder, level curLev, const fmtItrs& state);
        inline logger(const logger& other);
        inline ~logger();

        inline void putFmtStr();

        llogger& holder_;
        std::ostringstream buf_;
        fmtItrs state_;
        level curLev_;

        inline void putFmtStr(fmtItrs& state);
        inline void timeStamp(fmtItrs& state);
        inline void putLogLev(fmtItrs& state);
        inline void putFmtLmb(fmtItrs& state);
    };

    template <typename T>
    using isRetNonVoid =  std::enable_if_t<
        !std::is_same_v<void, invokeResult<T> >, bool
    >;

    friend llogger::logger&& operator << (llogger::logger&& wrap, llogger::fmtStrType);
    
    template <typename T, typename llogger::isRetNonVoid<T>>
    friend logger&& operator << (llogger::logger&& wrap, const T& content);
    template <typename T, typename std::enable_if<!isCallable<T>::value, bool>::type>
    friend logger&& operator << (llogger::logger&& wrap, const T& content);

    bool enable_;
    const llfmt& fmt;
    std::ostream& os_;
    level level_;
    level curLev;

    static const llfmt& defaultFmt();

  public:
    inline logger operator() ();
    inline logger operator() (llogger::level lev);
    inline logger operator() (bool predicate);
    inline logger operator() (llogger::level lev, bool predicate);

    template<typename T = std::chrono::microseconds>
    static inline long long tElapsed(const std::chrono::steady_clock::time_point& start);
};

const llfmt& llogger::defaultFmt(){
    static const llfmt ret = llfmt(
            llfmt() << "[" << llfmt::time  << "] "
                    << llfmt::level << ": "
                    << llfmt::logStr
    );
    return ret;
}

llogger::llogger(std::ostream& os, level lev, const llfmt& format): os_(os), 
                                                                    level_(lev),
                                                                    curLev(llogger::info),
                                                                    fmt(format){
};

llogger::llogger(const llogger& other): os_(other.os_),
                                        level_(other.level_),
                                        curLev(llogger::info),
                                        fmt(other.fmt){
}


llogger::logger::logger(llogger& holder, 
                        level curLev, 
                        const fmtItrs& state):  holder_(holder),
                                                curLev_(curLev),
                                                state_(state){
    llogger::logger::putFmtStr();
}

llogger::logger::logger(const logger& other): holder_(other.holder_),
                                            curLev_(other.curLev_),
                                            state_(other.state_){
}

llogger::logger::~logger(){
    if(holder_.enable_){
        holder_.os_ << buf_.str() << std::endl;
    }
}

void llogger::logger::putFmtStr(fmtItrs& state){
    buf_ << *(state.fmtStrIter++);
}

void llogger::logger::timeStamp(fmtItrs& state){
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    buf_ << std::put_time(std::localtime(&now_time), " %Y-%m-%d %X ");
}

void llogger::logger::putLogLev(fmtItrs& state){
    buf_ << holder_.fmt.levelNames_[static_cast<size_t>(curLev_)];
}

void llogger::logger::putFmtLmb(fmtItrs& state){
    buf_ << (*(state.fmtLmbIter++))();
}

llogger::logger llogger::operator() (){
    enable_ = curLev <= level_;
    return logger(*this, curLev, fmt.getIters());
}

llogger::logger llogger::operator() (llogger::level lev){
    curLev = lev;
    enable_ = curLev <= level_;
    return logger(*this, curLev, fmt.getIters());
}

llogger::logger llogger::operator() (bool predicate){
    enable_ = curLev <= level_ && predicate;
    return logger(*this, curLev, fmt.getIters());
}

llogger::logger llogger::operator() (llogger::level lev, bool predicate){
    curLev = lev;
    enable_ = curLev <= level_ && predicate;
    return logger(*this, curLev, fmt.getIters());
}

template<typename T>
long long llogger::tElapsed(const std::chrono::steady_clock::time_point& start){
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<T>(end - start).count();
}

void llogger::logger::putFmtStr(){
    static const std::array<void (llogger::logger::*)(fmtItrs& state), 4> fmtCbs{
        &logger::putFmtStr, 
        &logger::putLogLev, 
        &logger::timeStamp, 
        &logger::putFmtLmb
    };

    if(holder_.enable_){
        for(size_t i: holder_.fmt.fmtOpt(state_)){
            (this->*(fmtCbs[i]))(state_);
        }
    }
}

llogger::logger&& operator << (llogger::logger&& wrap, llogger::fmtStrType){
    wrap.putFmtStr();
    return std::move(wrap);
}

template <typename T, typename llogger::isRetNonVoid<T> = true>
llogger::logger&& operator << (llogger::logger&& wrap, const T& content){
    if(wrap.holder_.enable_){
        wrap.buf_ << content();
    }
    return std::move(wrap); 
}

template <typename T, 
    typename std::enable_if<!isCallable<T>::value, bool>::type = true>
llogger::logger&& operator << (llogger::logger&& wrap, const T& content){
    if(wrap.holder_.enable_){
        wrap.buf_ << content;
    }
    return std::move(wrap);
}