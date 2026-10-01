#include "Locale.h"

namespace mrm {

const char* Locale::operator[](uint16_t key) const {
    if (key >= count_)
        return "?";
    return lang_ == Lang::En ? table_[key].en : table_[key].ptBr;
}

const char* Locale::code(Lang lang) {
    return lang == Lang::En ? "en" : "pt-BR";
}

const char* Locale::nativeName(Lang lang) {
    return lang == Lang::En ? "English" : "Português";
}

bool Locale::fromCode(const char* code, Lang& out) {
    if (!code)
        return false;
    if (strncasecmp(code, "pt", 2) == 0)
        out = Lang::PtBr;
    else if (strncasecmp(code, "en", 2) == 0)
        out = Lang::En;
    else
        return false;
    return true;
}

} // namespace mrm
