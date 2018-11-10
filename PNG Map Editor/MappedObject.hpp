#pragma once

#include <string>

struct MappedObject {

    struct Type {

        enum Enum {

            Null = 0,
            Instance = 1,
            Tile = 2

        };

    };

    std::string name;

    bool centered;
    bool can_stretch;

    Type::Enum type;

    MappedObject(const std::string & name, Type::Enum type, bool centered, bool can_stretch)
        : name(name), type(type), centered(centered), can_stretch(can_stretch) { }

    MappedObject()
        : name(""), type(Type::Null), centered(false), can_stretch(true) { }

    ~MappedObject() { }

};

struct PositionedMappedObject {

    unsigned x, y, xscale, yscale;
    MappedObject obj;

    PositionedMappedObject(unsigned x, unsigned y, unsigned xscale, unsigned yscale, const MappedObject & obj)
        : x(x), y(y), xscale(xscale), yscale(yscale), obj(obj) { }

    ~PositionedMappedObject() { }

};