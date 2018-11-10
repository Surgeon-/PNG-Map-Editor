
#include "pch.h"

#define _CRT_SECURE_NO_WARNINGS

#include "Converter.hpp"

#include <iostream>

#include <algorithm> // min, max
#include <deque>
#include <fstream>
#include <vector>
#include <regex>
#include <functional>

#include <SFML\Graphics.hpp>
#include <SFML\System.hpp>

#include <stdexcept>

#include "MappedObject.hpp"

namespace {

    std::string string_crop(const std::string & str, const std::string & tokens = " \t") {

        if (str.empty()) return std::string("");

        size_t head = 0;
        size_t tail = str.size() - 1;

        while (tokens.find(str[head]) != std::string::npos)
            head += 1;

        while (tail >= head && tokens.find(str[tail]) != std::string::npos)
            tail -= 1;

        if (tail < head) return std::string("");

        return std::string(str.c_str() + head, tail - head + 1);

    }

    bool coleq(const sf::Color & col1, const sf::Color & col2, bool ignore_alpha = false) {

        if (!ignore_alpha) {

            return (col1 == col2);

        } else {

            return (col1.r == col2.r && col1.g == col2.g && col1.b == col2.b);

        }

    }

    bool receq(const sf::Image & img, size_t x1, size_t y1, size_t x2, size_t y2, sf::Color ref) {

        for (size_t i = x1; i <= x2; i += 1) 
            for (size_t t = y1; t <= y2; t += 1) {

                if (!coleq(img.getPixel(i, t), ref, true)) return false;

            }

        return true;

    }

    void find_max_rec(const sf::Image & img, size_t x, size_t y, size_t limit, size_t & out_w, size_t & out_h, sf::Color ref) {

        const size_t max_x = std::min({x + limit, img.getSize().x});
        const size_t max_y = std::min({y + limit, img.getSize().y});

        out_w = 1;
        out_h = 1;

        size_t max_surf = 1;

        for (size_t i = x; i < max_x; i += 1) {
            for (size_t t = y; t < max_y; t += 1) {

                if ((i - x + 1) * (t - y + 1) >= max_surf && receq(img, x, y, i, t, ref)) {

                    out_w = i - x + 1;
                    out_h = t - y + 1;
                    max_surf = out_w * out_h;

                }

            }
        }

    }

    void recclr(sf::Image & img, size_t x, size_t y, size_t w, size_t h, sf::Color col) {

        for (size_t i = x; i < x + w; i += 1)
            for (size_t t = y; t < y + h; t += 1) {

                img.setPixel(i, t, col);

            }

    }

    std::string system_error_str() {
    
        char buffer[256];

        strerror_s(buffer, (sizeof(buffer) / sizeof(buffer[0])), errno);

        return std::string(buffer);
    
    }

    bool match_empty_line(const std::string & str) {
    
        static const char * pattern = R"(^\s*$)";

        return std::regex_match(str, std::regex(pattern));

    }

    bool match_mapping(const std::string & str, std::vector<std::string> & outvec, size_t * flag_cnt = nullptr) {

        static const
        char * pattern = R"(^\s*([[:digit:]]+|0x[[:xdigit:]]+)\s*,)"             // First number and comma
                         R"(\s*([[:digit:]]+|0x[[:xdigit:]]+)\s*,)"              // Second number and comma
                         R"(\s*([[:digit:]]+|0x[[:xdigit:]]+)\s*->\s*)"          // Third number and arrow
                         R"((INST|TILE)\s*\(\s*)"                                // Type and opening parenthesis
                         R"(([_a-zA-Z]+\w*))"                                    // Name
                         R"((\s*;\s*([_a-zA-Z]+\w*)(\s*,\s*([_a-zA-Z]+\w*))*)?)" // Optional semicolon & flags
                         R"(\s*\)\s*$)" ;                                        // Closing parenthesis and end of line     

        std::smatch res;

        bool match = std::regex_match(str, res, std::regex(pattern));

        if (!match) return false;

        outvec.clear();

        outvec.push_back(res[1].str()); // R
        outvec.push_back(res[2].str()); // G
        outvec.push_back(res[3].str()); // B
        outvec.push_back(res[4].str()); // Type
        outvec.push_back(res[5].str()); // Name

        if (res[6].matched) { 

            std::string flags{res[6].str()};

            auto lambda = [](char c){ return bool(c == ';' || ::isspace(c)); };

            flags.erase(std::remove_if(flags.begin(), flags.end(), lambda), flags.end());

            char * ctx = NULL;
            char * buf = &flags[0];
            char * pch = strtok(buf, ",");

            while (pch != NULL) { // Tokenize and insert flags

                outvec.emplace_back(pch);
                pch = strtok(NULL, ",");

            }

            if (flag_cnt != nullptr) *flag_cnt = (outvec.size() - 5u);


        }

        return true;

    }

    bool match_header_path(const std::string & str, std::string & path) {

        static const
        char * pattern = R"(^\s*SET_HEADER_PATH\s+((.)+)$)";

        std::smatch res;

        bool match = std::regex_match(str, res, std::regex(pattern));

        if (match) {

            path = string_crop(res[1].str());

            return true;

        }

        return false;

    }
    
    bool match_footer_path(const std::string & str, std::string & path) {
    
        static const
        char * pattern = R"(^\s*SET_FOOTER_PATH\s+((.)+)$)";

        std::smatch res;

        bool match = std::regex_match(str, res, std::regex(pattern));

        if (match) {

            path = string_crop(res[1].str());

            return true;

        }

        return false;

    }

    bool match_resolution(const std::string & str, unsigned & resolution) {

        static const
        char * pattern = R"(^\s*SET_RESOLUTION\s+([[:digit:]]+)\s*$)";

        std::smatch res;

        bool match = std::regex_match(str, res, std::regex(pattern));

        if (match) {

            unsigned rv = std::stoi(res[1].str());

            resolution = rv;

            return true;

        }

        return false;

    }

    bool match_max_scale(const std::string & str, unsigned & max_scale) {

        static const
            char * pattern = R"(^\s*SET_MAX_SCALE\s+([[:digit:]]+)\s*$)";

        std::smatch res;

        bool match = std::regex_match(str, res, std::regex(pattern));

        if (match) {

            unsigned rv = std::stoi(res[1].str());

            max_scale = rv;

            return true;

        }

        return false;

    }

    const unsigned MAP_OUTVEC_RED    = 0u;
    const unsigned MAP_OUTVEC_GREEN  = 1u;
    const unsigned MAP_OUTVEC_BLUE   = 2u;
    const unsigned MAP_OUTVEC_TYPE   = 3u;
    const unsigned MAP_OUTVEC_NAME   = 4u;
    const unsigned MAP_OUTVEC_FLAG_0 = 5u;

}

// PUBLIC /////////////////////////////////////////////////////////////////////

void Converter::convertImage(const sf::Image & img) const {

    if (m_limit == 0) {
        throw std::runtime_error("Converter::convertImage - You must set MaxScale to a non-zero value.");
    }

    if (m_resolution == 0) {
        throw std::runtime_error("Converter::convertImage - You must set Resolution to a non-zero value.");
    }

    if (m_map.empty()) {
        throw std::runtime_error("Converter::convertImage - No mappings defined.");
    }

    std::deque<PositionedMappedObject> instances;
    std::deque<PositionedMappedObject> tiles;

    std::ifstream header{m_header_path.toWideString(), std::ifstream::in};
    if (!header) {
        throw std::runtime_error("Converter::convertImage - Can't open Header file for reading: " + system_error_str());
    }

    std::ifstream footer{m_footer_path.toWideString(), std::ifstream::in};
    if (!footer) {
        throw std::runtime_error("Converter::convertImage - Can't open Footer file for reading: " + system_error_str());
    }

    std::ofstream output{m_output_path.toWideString(), std::ofstream::out | std::ofstream::trunc};
    if (!output) {
        throw std::runtime_error("Converter::convertImage - Can't open Output file for writing: " + system_error_str());
    }

    readImage(img, instances, tiles);

    std::string line;

    // Write header:
    while (std::getline(header, line)) {
        output << line << "\n";
    }

    // Write instances:
    writeInstances(output, instances);

    // Write tiles:
    writeTiles(output, tiles);

    // Write footer:
    while (std::getline(footer, line)) {
        output << line << "\n";
    }

    // Finalize:
    output.flush();

    if (!output) {
        throw std::runtime_error("Converter::convertImage - Encountered an error while writing to Output file.");
    }

}

void Converter::setMapping(const sf::Color & col, const MappedObject & obj) {

    m_map[ col.toInteger() ] = obj;

}

void Converter::clearMappings() {

    m_map.clear();

}

void Converter::configureFromFile(const sf::String & path) {

    std::string line, outstr;
    std::vector<std::string> outvec;
    size_t flag_cnt;
    unsigned out_uint;

    std::ifstream file{path.toWideString(), std::ifstream::in};
    if (!file) {
        throw std::runtime_error("Converter::configureFromFile - Can't open Config file for reading: " + system_error_str());
    }

    while (std::getline(file, line)) {
    
        if (match_empty_line(line)) {
            // Skip
        }
        else if (match_resolution(line, out_uint)) {
            setResolution(out_uint);
        }
        else if (match_max_scale(line, out_uint)) {
            setMaxScale(out_uint);
        }
        else if (match_mapping(line, outvec, &flag_cnt)) {
        
            if (!mapFromVector(outvec, flag_cnt, outstr)) {
                throw std::runtime_error("Converter::configureFromFile - Mapping error: " + outstr);
            }

        }
        else if (match_header_path(line, outstr)) {
            m_header_path = outstr;
        }
        else if (match_footer_path(line, outstr)) {
            m_footer_path = outstr;
        }
        else {
            throw std::runtime_error("Converter::configureFromFile - Can't interpret line [" + line + "].");
        }
    
    }

}

// PRIVATE ////////////////////////////////////////////////////////////////////

bool Converter::mapFromVector(const std::vector<std::string> & vec, size_t flag_cnt, std::string & outstr) {

    int r = std::stoi( vec[MAP_OUTVEC_RED]   );
    int g = std::stoi( vec[MAP_OUTVEC_GREEN] );
    int b = std::stoi( vec[MAP_OUTVEC_BLUE]  );

    if (r < 0 || g < 0 || b < 0 || r > 255 || g >255 || b > 255) {
        
        outstr = "Colour out of range [must be 0-255].";
        return false;

    }

    std::string name = vec[MAP_OUTVEC_NAME];
    std::string type = vec[MAP_OUTVEC_TYPE];

    MappedObject::Type::Enum etype;

    if (type == "INST")
        etype = MappedObject::Type::Instance;
    else if (type == "TILE")
        etype = MappedObject::Type::Tile;
    else {
    
        outstr = "Unknown object type [" + type + "].";
        return false;
    
    }

    bool f_cent = false, f_strc = false;

    for (size_t i = 0; i < flag_cnt; i += 1) {
    
        std::string flag = vec[MAP_OUTVEC_FLAG_0 + i];

        if (flag == "CENT") { f_cent = true; continue; }
        if (flag == "STRC") { f_strc = true; continue; }

        outstr = "Unknown flag [" + flag + "].";
        return false;

    }

    setMapping( sf::Color(sf::Uint8(r), sf::Uint8(g), sf::Uint8(b))
              , MappedObject(name, etype, f_cent, f_strc)
              ) ;

    return true;

}

void Converter::readImage(const sf::Image & img, std::deque<PositionedMappedObject> & instances, std::deque<PositionedMappedObject> & tiles) const {

    #define C_WHITE  sf::Color(255u, 255u, 255u)
    #define STR(arg) std::to_string(arg)

    sf::Image temp{img};

    const size_t iw = temp.getSize().x;
    const size_t ih = temp.getSize().y;

    instances.clear();
    tiles.clear();

    for (size_t i = 0; i < iw; i += 1) {
        for (size_t t = 0; t < ih; t += 1) {

            auto curr = temp.getPixel(i, t);

            // Ignore whites:
            if (coleq(curr, C_WHITE, true)) continue;
            //printf("Checking non-white pixel at (%d, %d).\n", int(i), int(t));

            size_t w, h;

            find_max_rec(temp, i, t, m_limit, w, h, curr);
            //printf("    MaxRect: w = %d, h = %d\n", w, h);

            recclr(temp, i, t, w, h, C_WHITE);

            //printf( "    New instance: x = %d, y = %d, w = %d, h = %d, col = %X \n", i * 32, t * 32, w, h, curr.toInteger() );

            MappedObject obj;

            if (!colToObj(curr, obj)) 
                throw std::runtime_error("Converter::readImage - Could not map pixel at (x=" + STR(i) + ", y=" + STR(t) + ") to an object.");

            switch (obj.type) {
            
            case MappedObject::Type::Instance:
                instances.emplace_back(i * m_resolution, t * m_resolution, w, h, obj);
                break;

            case MappedObject::Type::Tile:
                tiles.emplace_back(i * m_resolution, t * m_resolution, w, h, obj);
                break;

            default:
                throw std::runtime_error("Converter::readImage - Encountered MappedObject of unknown type.");
                break;
            
            }

        }
    }

    #undef STR
    #undef C_WHITE

}

bool Converter::colToObj(const sf::Color & col, MappedObject & obj) const {

    auto iter = m_map.find(col.toInteger());

    if (iter == m_map.end()) return false;

    obj = ((*iter).second);

    return true;

 }

void Converter::writeInstances(std::ostream & os, const std::deque<PositionedMappedObject> & inst) const {

    sf::Uint32 counter = (1000 * 1000 * 1000);
    char uidbuf[32];
    std::vector<char> buffer;
    buffer.resize(512u, '\0');

    os << "<instances>\n";

    for (const PositionedMappedObject & item : inst) {
    
        if (item.obj.can_stretch) {

            snprintf(uidbuf, 32u, "inst_%X", counter);
            counter += 1;

            int xx = int(item.x) + ((item.obj.centered) ? (m_resolution * item.xscale / 2) : (0));
            int yy = int(item.y) + ((item.obj.centered) ? (m_resolution * item.yscale / 2) : (0));

            snprintf( &buffer[0], 512u,
                      "<instance objName=\"%s\" x=\"%d\" y=\"%d\" name=\"%s\" locked=\"0\" code=\"\" "
                      "scaleX=\"%d\" scaleY=\"%d\" colour=\"4294967295\" rotation=\"0\"/>",
                      // object, x, y, uid, xscale, yscale
                      item.obj.name.c_str(), xx, yy, uidbuf, int(item.xscale), int(item.yscale)
                    ) ;

            os << &buffer[0] << "\n";

        }
        else {

            for (unsigned i = 0; i < item.xscale; i += 1)
                for (unsigned t = 0; t < item.yscale; t += 1) {
                
                    snprintf(uidbuf, 32u, "inst_%X", counter);
                    counter += 1;

                    int xx = int(item.x) + i * m_resolution + ((item.obj.centered) ? (m_resolution / 2) : (0));
                    int yy = int(item.y) + t * m_resolution + ((item.obj.centered) ? (m_resolution / 2) : (0));
                
                    snprintf( &buffer[0], 512u,
                              "<instance objName=\"%s\" x=\"%d\" y=\"%d\" name=\"%s\" locked=\"0\" code=\"\" "
                              "scaleX=\"%d\" scaleY=\"%d\" colour=\"4294967295\" rotation=\"0\"/>",
                              // object, x, y, uid, xscale, yscale
                              item.obj.name.c_str(), xx, yy, uidbuf, 1, 1
                            ) ;

                    os << &buffer[0] << "\n";

                }

        }

    }

    os << "</instances>\n";

}

void Converter::writeTiles(std::ostream & os, const std::deque<PositionedMappedObject> & tiles) const {

    os << "<tiles>\n";

    for (const auto & item : tiles) {

        throw std::logic_error("Converter::writeTiles - Tiles are not yet supported.");

    }

    os << "</tiles>\n";

}