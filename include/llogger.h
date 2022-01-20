/*
  Copyright (C) 2020-2022 SdtElectronics <null@std.uestc.edu.cn>
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
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "lldefs.h"
#include "osSync.hpp"

namespace ll{

namespace detail{
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

template<typename B>
struct logger;

struct fmtItrs{
    using fmtCallback = std::function<std::string(void)>;
    std::vector<std::string>::const_iterator          fmtStrIter;
    std::vector<std::vector<size_t> >::const_iterator fmtOrdIter;
    std::vector<fmtCallback>::const_iterator          fmtLmbIter;
};

} // namespace detail

class llfmt{
  public:
    enum infoType: char{level = 1, time};
    enum dataType: char{logStr};
    using fmtCallback = std::function<std::string(void)>;
    using levelStrArr = std::array<const char *, levels>;

    inline llfmt(const levelStrArr& levelNames = defaultLevelStr());

    inline const std::vector<size_t>& fmtOpt(detail::fmtItrs& state) const;
    inline detail::fmtItrs getIters() const;

    inline llfmt& operator << (llfmt::infoType info);
    inline llfmt& operator << (llfmt::dataType);
    inline llfmt& operator << (const char* fmtStr);
    inline llfmt& operator << (const std::string& fmtStr);
    inline llfmt& operator << (const fmtCallback& fmtLmb);

  private:
    template<typename>
    friend class llogger;

    template<typename>
    friend class detail::logger;
    
    std::vector<std::vector<size_t> > fmtOrds;
    std::vector<std::string>          fmtStrs;
    std::vector<fmtCallback>          fmtLmbs;

    const levelStrArr& levelNames_;

    inline static const levelStrArr& defaultLevelStr();
};

const std::vector<size_t>& llfmt::fmtOpt(detail::fmtItrs& state) const{
    return *(state.fmtOrdIter++);
}

detail::fmtItrs llfmt::getIters() const{
    return {
        fmtStrs.cbegin(),
        fmtOrds.cbegin(),
        fmtLmbs.cbegin()
    };
}

llfmt::llfmt(const levelStrArr& levelNames): levelNames_(levelNames),
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

const llfmt::levelStrArr& llfmt::defaultLevelStr(){
    static constexpr llfmt::levelStrArr ret{
        "\033[1m\033[31m FATAL \033[0m", 
        "\033[1m\033[31m ERROR \033[0m", 
        "\033[1m\033[35mWARNING\033[0m", 
        "\033[1m\033[34mNOTICE \033[0m", 
        "\033[1m\033[36m INFO  \033[0m",
                       " DEBUG "
    };

    return ret;
};

enum fmtStrType: char{fmtStr};

template<typename B = OStreamSync>
class llogger{
  public:
    llogger(level lev, B& backend = defaultBackend(), const llfmt& format = defaultFmt());
    llogger(const llogger<B>& other);

  private:
    template<typename>
    friend class detail::logger;

    template <typename T>
    using isRetNonVoid =  typename std::enable_if<
        !std::is_same<void, detail::invokeResult<T> >::value, bool
    >::type;
    
    template<typename T, typename B_, typename llogger<B_>::template isRetNonVoid<T> >
    friend detail::logger<B_>&& operator << (detail::logger<B_>&& wrap, const T& content);
    template<typename T, typename B_, typename std::enable_if<!detail::isCallable<T>::value, bool>::type>
    friend detail::logger<B_>&& operator << (detail::logger<B_>&& wrap, const T& content);

    const llfmt& fmt;
    B& backend_;
    level level_;
    level curLev;

    static const llfmt& defaultFmt();
    static OStreamSync& defaultBackend();

  public:
    inline detail::logger<B> operator() ();
    inline detail::logger<B> operator() (level lev);
    inline detail::logger<B> operator() (bool predicate);
    inline detail::logger<B> operator() (level lev, bool predicate);

    template<typename T = std::chrono::microseconds>
    static inline long long tElapsed(const std::chrono::steady_clock::time_point& start);
};

template<typename B>
const llfmt& llogger<B>::defaultFmt(){
    static const llfmt ret = llfmt(
            llfmt() << "[" << llfmt::time  << "] "
                    << llfmt::level << ": "
                    << llfmt::logStr
    );
    return ret;
}

template<typename B>
OStreamSync& llogger<B>::defaultBackend(){
    static OStreamSync ret(std::cout);
    return ret;
}

template<typename B>
llogger<B>::llogger(level lev, B& backend, const llfmt& format): backend_(backend),
                                                                level_(lev),
                                                                curLev(info),
                                                                fmt(format){
};

template<typename B>
llogger<B>::llogger(const llogger& other): backend_(other.backend_),
                                        level_(other.level_),
                                        curLev(info),
                                        fmt(other.fmt){
}


template<typename B>
detail::logger<B> llogger<B>::operator() (){
    bool enable = curLev <= level_;
    return detail::logger<B>(*this, enable, curLev, fmt.getIters());
}

template<typename B>
detail::logger<B> llogger<B>::operator() (level lev){
    curLev = lev;
    bool enable = curLev <= level_;
    return detail::logger<B>(*this, enable, curLev, fmt.getIters());
}

template<typename B>
detail::logger<B> llogger<B>::operator() (bool predicate){
    bool enable = curLev <= level_ && predicate;
    return detail::logger<B>(*this, enable, curLev, fmt.getIters());
}

template<typename B>
detail::logger<B> llogger<B>::operator() (level lev, bool predicate){
    curLev = lev;
    bool enable = curLev <= level_ && predicate;
    return detail::logger<B>(*this, enable, curLev, fmt.getIters());
}

template<typename B>
template<typename T>
long long llogger<B>::tElapsed(const std::chrono::steady_clock::time_point& start){
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<T>(end - start).count();
}

namespace detail{

template<typename B>
struct logger{
    inline logger(llogger<B>& holder, bool enable, level curLev, const fmtItrs& state);
    inline logger(const logger<B>& other);

    template <typename BS = B, typename std::enable_if<
        isCallable<decltype(&BS::log), BS&, const std::string&>::value, bool>::type = true>
    inline void dtorImpl();
    template <typename BS = B,typename std::enable_if<
        isCallable<decltype(&BS::log), BS&, const std::string&, level>::value, bool>::type = true>
    inline void dtorImpl();
    inline ~logger();

    inline void putFmtStr();

    llogger<B>& holder_;
    std::ostringstream buf_;
    bool enable_;
    level curLev_;
    fmtItrs state_;

    inline void putFmtStr(fmtItrs& state);
    inline void timeStamp(fmtItrs& state);
    inline void putLogLev(fmtItrs& state);
    inline void putFmtLmb(fmtItrs& state);
};

template<typename B>
logger<B>::logger(llogger<B>& holder, 
                bool enable, 
                level curLev, 
                const fmtItrs& state):  
                holder_(holder),
                enable_(enable),
                curLev_(curLev),
                state_(state){
    logger<B>::putFmtStr();
}

template<typename B>
logger<B>::logger(const logger<B>& other):  holder_(other.holder_),
                                            curLev_(other.curLev_),
                                            state_(other.state_){
}


template<typename B>
template <typename BS, typename std::enable_if<
    isCallable<decltype(&BS::log), BS&, const std::string&>::value, bool>::type>
void logger<B>::dtorImpl(){
    holder_.backend_.log(buf_.str());
}

template<typename B>
template <typename BS, typename std::enable_if<
    isCallable<decltype(&BS::log), BS&, const std::string&, level>::value, bool>::type>
void logger<B>::dtorImpl(){
    holder_.backend_.log(buf_.str(), curLev_);
}

template<typename B>
logger<B>::~logger(){
    if(enable_){
        dtorImpl();
    }
}

template<typename B>
void logger<B>::putFmtStr(fmtItrs& state){
    buf_ << *(state.fmtStrIter++);
}

template<typename B>
void logger<B>::timeStamp(fmtItrs& state){
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    buf_ << std::put_time(std::localtime(&now_time), " %Y-%m-%d %X ");
}

template<typename B>
void logger<B>::putLogLev(fmtItrs& state){
    buf_ << holder_.fmt.levelNames_[static_cast<size_t>(curLev_)];
}

template<typename B>
void logger<B>::putFmtLmb(fmtItrs& state){
    buf_ << (*(state.fmtLmbIter++))();
}

template<typename B>
void logger<B>::putFmtStr(){
    static const std::array<void (logger<B>::*)(fmtItrs& state), 4> fmtCbs{
        &logger::putFmtStr, 
        &logger::putLogLev, 
        &logger::timeStamp, 
        &logger::putFmtLmb
    };

    if(enable_){
        for(size_t i: holder_.fmt.fmtOpt(state_)){
            (this->*(fmtCbs[i]))(state_);
        }
    }
}

template<typename B>
logger<B>&& operator << (logger<B>&& wrap, fmtStrType){
    wrap.putFmtStr();
    return std::move(wrap);
}

template<typename T, typename B, typename llogger<B>::template isRetNonVoid<T> = true>
logger<B>&& operator << (logger<B>&& wrap, const T& content){
    if(wrap.enable_){
        wrap.buf_ << content();
    }
    return std::move(wrap); 
}

template<typename T, typename B,
    typename std::enable_if<!isCallable<T>::value, bool>::type = true>
logger<B>&& operator << (logger<B>&& wrap, const T& content){
    if(wrap.enable_){
        wrap.buf_ << content;
    }
    return std::move(wrap);
}

} // namespace detail

} // namespace ll