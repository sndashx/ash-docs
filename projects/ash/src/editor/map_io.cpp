#include "editor/map_io.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace ash {
namespace editor {

namespace {

std::string entity_kind_str(EntityKind k) noexcept {
    switch (k) {
    case EntityKind::NPC:       return "npc";
    case EntityKind::Item:      return "item";
    case EntityKind::Container: return "container";
    case EntityKind::Door:      return "door";
    case EntityKind::Trigger:   return "trigger";
    case EntityKind::Light:     return "light";
    case EntityKind::Sign:      return "sign";
    case EntityKind::Count:     break;
    }
    return "trigger";
}

bool parse_entity_kind(std::string const& s, EntityKind& out) {
    if (s == "npc")       { out = EntityKind::NPC;       return true; }
    if (s == "item")      { out = EntityKind::Item;      return true; }
    if (s == "container") { out = EntityKind::Container; return true; }
    if (s == "door")      { out = EntityKind::Door;      return true; }
    if (s == "trigger")   { out = EntityKind::Trigger;   return true; }
    if (s == "light")     { out = EntityKind::Light;     return true; }
    if (s == "sign")      { out = EntityKind::Sign;      return true; }
    return false;
}

bool parse_hex_byte(std::string const& s, std::uint8_t& out) {
    if (s.size() < 2) return false;
    std::size_t pos = 0;
    unsigned long v = 0;
    try { v = std::stoul(s, &pos, 16); } catch (...) { return false; }
    if (pos == 0) return false;
    if (v > 0xFFul) return false;
    out = static_cast<std::uint8_t>(v);
    return true;
}

bool parse_hex_u32(std::string const& s, std::uint32_t& out) {
    if (s.empty()) return false;
    std::size_t pos = 0;
    unsigned long v = 0;
    try { v = std::stoul(s, &pos, 16); } catch (...) { return false; }
    if (pos == 0) return false;
    out = static_cast<std::uint32_t>(v);
    return true;
}

}  // namespace

bool save_map(Map const& m, std::string const& path) {
    std::ofstream out(path);
    if (!out) return false;
    out << "ASH-MAP-V1 " << m.width << " " << m.height << " " << m.map_id << "\n";
    for (int layer = 1; layer <= kLayerCount; ++layer) {
        out << "L " << layer << "\n";
        for (int y = 0; y < m.height; ++y) {
            for (int x = 0; x < m.width; ++x) {
                render::Cell c = m.at(layer, x, y);
                char buf[96];
                std::snprintf(buf, sizeof(buf), "%08x %02x%02x%02x %02x%02x%02x %02x",
                              static_cast<unsigned>(c.glyph),
                              c.fg_r, c.fg_g, c.fg_b,
                              c.bg_r, c.bg_g, c.bg_b,
                              static_cast<unsigned>(c.flags));
                out << buf;
                out << (x + 1 == m.width ? '\n' : ' ');
            }
        }
    }
    for (auto const& e : m.entities) {
        out << "E " << e.id << " " << entity_kind_str(e.kind)
            << " " << e.type_id
            << " " << e.pos.x << " " << e.pos.y
            << " " << e.layer
            << " " << e.label << "\n";
    }
    return static_cast<bool>(out);
}

bool load_map(Map& m, std::string const& path) {
    std::ifstream in(path);
    if (!in) return false;
    std::string header;
    if (!(in >> header)) return false;
    if (header != "ASH-MAP-V1") return false;

    Map next;
    int w = 0, h = 0;
    if (!(in >> w >> h)) return false;
    in >> std::ws;
    std::getline(in, next.map_id);
    next.resize(w, h);

    std::string token;
    while (in >> token) {
        if (token == "L") {
            int layer = 0;
            if (!(in >> layer)) return false;
            if (layer < 1 || layer > kLayerCount) return false;
            in >> std::ws;
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    std::string glyph_s, fg_s, bg_s, flag_s;
                    if (!(in >> glyph_s >> fg_s >> bg_s >> flag_s)) return false;
                    std::uint32_t glyph = 0;
                    std::uint8_t  fg_r = 0, fg_g = 0, fg_b = 0;
                    std::uint8_t  bg_r = 0, bg_g = 0, bg_b = 0;
                    std::uint8_t  flags = 0;
                    if (!parse_hex_u32(glyph_s, glyph)) return false;
                    if (fg_s.size() != 6) return false;
                    if (bg_s.size() != 6) return false;
                    if (!parse_hex_byte(fg_s.substr(0, 2), fg_r)) return false;
                    if (!parse_hex_byte(fg_s.substr(2, 2), fg_g)) return false;
                    if (!parse_hex_byte(fg_s.substr(4, 2), fg_b)) return false;
                    if (!parse_hex_byte(bg_s.substr(0, 2), bg_r)) return false;
                    if (!parse_hex_byte(bg_s.substr(2, 2), bg_g)) return false;
                    if (!parse_hex_byte(bg_s.substr(4, 2), bg_b)) return false;
                    if (!parse_hex_byte(flag_s, flags)) return false;
                    render::Cell& dst = next.cell(layer, x, y);
                    dst.glyph = glyph;
                    dst.fg_r = fg_r; dst.fg_g = fg_g; dst.fg_b = fg_b;
                    dst.bg_r = bg_r; dst.bg_g = bg_g; dst.bg_b = bg_b;
                    dst.flags = flags;
                }
            }
        } else if (token == "E") {
            EntitySpec e;
            std::string kind_s;
            if (!(in >> e.id >> kind_s >> e.type_id
                     >> e.pos.x >> e.pos.y >> e.layer)) return false;
            std::getline(in, e.label);
            if (!e.label.empty() && e.label[0] == ' ') e.label.erase(0, 1);
            if (!parse_entity_kind(kind_s, e.kind)) e.kind = EntityKind::Trigger;
            next.entities.push_back(e);
        } else {
            // Skip unknown tag line.
            std::string dummy;
            std::getline(in, dummy);
        }
    }
    m = std::move(next);
    return true;
}

}  // namespace editor
}  // namespace ash