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
#include <iostream>

class llogger{
    public:

        enum level: char{silent = 0, error, warning, info, verbose, all};

        llogger(std::ostream& os, level lev);
        template <typename T>
        llogger& operator << (const T& content);
        llogger& operator << (level content);

    private:
        template <typename T>
        void opImpl(const T& content, std::true_type tp);
        template <typename T>
        void opImpl(const T& content, std::false_type tp);
        std::ostream& _os;
        level _level;
        level curLev;
};

template <typename T, typename = void>
struct is_callable : public std::false_type {};

template <typename T>
struct is_callable<
    T,
    std::enable_if_t<!std::is_same_v<void, std::invoke_result_t<T>>>>
    : public std::true_type {};

template <typename T>
void llogger::opImpl(const T& content, std::false_type tp){
    _os << content;
}

template <typename T>
void llogger::opImpl(const T& content, std::true_type tp){
    _os << content();
}

template <typename T>
llogger& llogger::operator << (const T& content){
    if (curLev <= _level){
        opImpl(content, is_callable<T>{});
    }
    return *this;
}
