// UTF-8 parsing from http://stackoverflow.com/questions/4775437/read-unicode-utf-8-file-into-wstring
// http://stackoverflow.com/questions/4804298/how-to-convert-wstring-into-string
#include <clocale>
#include <cstdlib>
#include <fstream>
#include <locale>
#include <iostream>
#include <string>
#include <vector>

#include "otc/util.h"
#include "otc/newick_tokenizer.h"

namespace otc {

const std::string readStrContentOfUTF8File(const std::string &filepath) {
#if defined WIDE_STR_VERSION
    const std::wstring utf8content = readWStrContentOfUTF8File(filepath);
    const std::locale empty_locale("");
    typedef std::codecvt<wchar_t, char, std::mbstate_t> converter_type;
    const converter_type & converter = std::use_facet<converter_type>(empty_locale);
    std::vector<char> to((unsigned long)utf8content.length() * (unsigned long)converter.max_length());
    std::mbstate_t state;
    const wchar_t* from_next;
    char* to_next;
    const converter_type::result result = converter.out(state,
                                                        utf8content.data(),
                                                        utf8content.data() + utf8content.length(),
                                                        from_next,
                                                        &to[0],
                                                        &to[0] + to.size(),
                                                        to_next);
    if (result == converter_type::ok or result == converter_type::noconv) {
        const std::string ccontent(&to[0], to_next);
        return ccontent;
    }
    throw OTCError("Error reading the contents of filepath as UTF-8");
#else
    std::ifstream inp;
    inp.open(filepath);
    if (!inp.good()) {
        throw OTCError("Could not open \"" + filepath + "\"");
    }
    const std::string utf8content((std::istreambuf_iterator<char>(inp) ),
                                    (std::istreambuf_iterator<char>()));
    return utf8content;
#endif
}

bool openUTF8File(const std::string &filepath, std::ifstream & inp) {
    inp.open(filepath);
    return inp.good();
}
#if defined WIDE_STR_VERSION
bool openUTF8WideFile(const std::string &filepath, std::wifstream & inp) {
    std::setlocale(LC_ALL, "");
    const std::locale empty_locale("");
    typedef std::codecvt<wchar_t, char, std::mbstate_t> converter_type;
    const converter_type & converter = std::use_facet<converter_type>(empty_locale);
    const std::locale utf8_locale = std::locale(empty_locale, &converter);
    inp.open(filepath);
    inp.imbue(utf8_locale);
    return inp.good();
}
#endif

std::list<std::string> readLinesOfFile(const std::string & filepath) {
    std::ifstream inp;
    if (!openUTF8File(filepath, inp)) {
        throw OTCError("Could not open file \"" + filepath + "\"");
    }
    std::list<std::string> lines;
    std::string line;
    while (getline(inp, line)) {
        auto stripped = strip_surrounding_whitespace(line);
        if (!stripped.empty()) {
            lines.push_back(stripped);
        }
    }
    return lines;
}
/*!
    Returns true if `o` points to a string that represents a long (and `o` has no other characters than the long).
    if n is not NULL, then when the function returns true, *n will be the long.
*/
bool char_ptr_to_long(const char *o, long *n)
    {
    if (o == nullptr) {
        return false;
    }
    if (strchr("0123456789-+", *o) != nullptr) {
        char * pEnd;
        const long i = strtol(o, &pEnd, 10);
        if (*pEnd != '\0') {
            return false;
        }
        if (n != NULL) {
            *n = i;
        }
        return true;
    }
    return false;
}

// splits a string by whitespace and push the graphical strings to the back of r.
//  Leading and trailing whitespace is lost ( there will be no empty strings added
//      to the list.
std::list<std::string> split_string(const std::string &s)
    {
    std::list<std::string> r;
    std::string current;
    for (const auto & c : s) {
        if (isgraph(c))
            current.append(1, c);
        else if (!current.empty()) {
            r.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) {
        r.push_back(current);
    }
    return r;
}


std::list<std::string> split_string(const std::string &s, const char delimiter)
    {
    std::list<std::string> r;
    std::string current;
    for (const auto & c : s) {
        if (c != delimiter) {
            current.append(1, c);
        } else {
            r.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) {
        r.push_back(current);
    }
    return r;
}

std::set<long> parseDelimSeparatedIDs(const std::string &str, const char delimiter) {
    std::set<long> ottIds;
    std::list<std::string> idList = split_string(str, delimiter);
    for (const auto & id : idList) {
        long p = stringToOttID(id);
        if (p < 0) 
	    throw OTCError()<<"Expecting an OTT Id.  Found: '"<<id<<"'";
        ottIds.insert(p);
    }
    return ottIds;
}

std::set<long> parseListOfOttIds(const std::string &fp) {
    std::ifstream inpf;
    if (!openUTF8File(fp, inpf)) {
        throw OTCError("Could not open list of OTT ids file \"" + fp + "\"");
    }
    std::string line;
    std::set<long> ottIds;
    try{
        while (getline(inpf, line)) {
            const auto stripped = strip_surrounding_whitespace(line);
            if (!stripped.empty()) {
                long p = stringToOttID(line);
                if (p < 0)
		    throw OTCError()<<"Expecting an OTT Id.  Found: '"<<line<<"'";
                ottIds.insert(p);
            }
        }
    } catch (...) {
        inpf.close();
        throw;
    }
    inpf.close();
    return ottIds;
}

std::list<std::set<long> > parseDesignatorsFile(const std::string &fp) {
    std::ifstream inpf;
    if (!openUTF8File(fp, inpf)) {
        throw OTCError("Could not open designators file \"" + fp + "\"");
    }
    std::string line;
    std::list<std::set<long> > allDesignators;
    try{
        while (getline(inpf, line)) {
            const auto stripped = strip_surrounding_whitespace(line);
            if (!stripped.empty()) {
                const auto words = split_string(stripped);
                if (words.size() < 2) {
                    std::string m = "Expecting >1 designator on each line. Found: ";
                    m +=  line;
                    throw OTCError(m);
                }
                std::set<long> designators;
                for (const auto & ds : words) {
                    long d;
                    if (!char_ptr_to_long(ds.c_str(), &d)) {
                        std::string m = "Expecting numeric designator. Found: ";
                        m += line;
                        throw OTCError(m);
                    }
                    designators.insert(d);
                }
                allDesignators.push_back(designators);
            }
        }
    } catch (...) {
        inpf.close();
        throw;
    }
    inpf.close();
    return allDesignators;
}

std::string filepathToFilename(const std::string &filepath) {
    auto p = filepath.find_last_of('/');
    if (p == std::string::npos) {
        return filepath;
    }
    return filepath.substr(1 + p);
}


}//namespace otc
