#pragma once
#include "axion/report.hpp"
#include <charconv>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace axion::life::acoustic {
inline Count pulse(std::string_view word) {
    if (word=="_") return 0;
    if (word.starts_with('@')) {
        Count value=0;auto [end,ec]=std::from_chars(word.data()+1,word.data()+word.size(),value,16);
        if (ec==std::errc{} && end==word.data()+word.size() && word.size()<=17) return value;
    } else if (Count sound=sound_pattern(word)) return wave(sound);
    throw std::invalid_argument("expected a word of up to 12 letters, _, or @ followed by a 64-bit hex pulse");
}
inline Count spoken_word(std::string word) {
    while (!word.empty() && (word.back() == '?' || word.back() == '.' || word.back() == ',' || word.back() == '!'))
        word.pop_back();
    for (char &c : word)
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
    return pulse(word);
}
inline std::string spell(Count x) {
    if (!x) return "_";
    Count original=unwave(x);auto text=transcribe(original);
    if (text.size()<=12 && !text.empty() && sound_pattern(text)==original) return text;
    std::ostringstream out;out<<'@'<<std::hex<<std::setw(16)<<std::setfill('0')<<x;return out.str();
}
}
