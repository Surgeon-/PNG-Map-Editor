#pragma once

#include <SFML\Graphics.hpp>
#include <SFML\System.hpp>

#include <unordered_map>
#include <deque>

#include "MappedObject.hpp"

class Converter {

public:

    Converter() { m_limit = m_resolution = 0u; }

    // Configuration

    void setMaxScale(unsigned scale) { m_limit = scale; }
    void setResolution(unsigned resolution) { m_resolution = resolution; }

    void setHeaderPath(const sf::String & path) { m_header_path = path; }
    void setFooterPath(const sf::String & path) { m_footer_path = path; }
    void setOutputPath(const sf::String & path) { m_output_path = path; }

    void setMapping(const sf::Color & col, const MappedObject & obj);
    void clearMappings();

    void configureFromFile(const sf::String & path);

    // Work

    void convertImage(const sf::Image & img) const;

private:

    std::unordered_map<sf::Uint32, MappedObject> m_map;

    unsigned m_limit;
    unsigned m_resolution;

    sf::String m_header_path;
    sf::String m_footer_path;
    sf::String m_output_path;

    bool mapFromVector(const std::vector<std::string> & vec, size_t flag_cnt, std::string & outstr);
    bool colToObj(const sf::Color & col, MappedObject & obj) const;
    void readImage(const sf::Image & img, std::deque<PositionedMappedObject> & instances, std::deque<PositionedMappedObject> & tiles) const;
    void writeInstances(std::ostream & os, const std::deque<PositionedMappedObject> & inst) const;
    void writeTiles(std::ostream & os, const std::deque<PositionedMappedObject> & tiles) const;

};