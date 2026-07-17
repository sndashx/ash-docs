#include "combat/armor.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>

namespace ash {
namespace combat {

char const* armor_slot_name(ArmorSlot s) noexcept {
    switch (s) {
        case ArmorSlot::Chest:  return "chest";
        case ArmorSlot::Helm:   return "helm";
        case ArmorSlot::Boots:  return "boots";
        case ArmorSlot::Gloves: return "gloves";
        case ArmorSlot::Shield: return "shield";
        case ArmorSlot::Count:  break;
    }
    return "unknown";
}

namespace {

ArmorSlot slot_from_string(std::string const& s) noexcept {
    if (s == "helm")   return ArmorSlot::Helm;
    if (s == "boots")  return ArmorSlot::Boots;
    if (s == "gloves") return ArmorSlot::Gloves;
    if (s == "shield") return ArmorSlot::Shield;
    return ArmorSlot::Chest;
}

int  num(json::Value const* v, int fb) noexcept {
    return (v && v->is_number()) ? v->as_int() : fb;
}
float numf(json::Value const* v, float fb) noexcept {
    return (v && v->is_number()) ? static_cast<float>(v->as_number()) : fb;
}
std::string str(json::Value const* v, std::string const& fb) noexcept {
    return (v && v->is_string()) ? v->as_string() : fb;
}

}  // namespace

ArmorSummary summarize(std::vector<ArmorDef const*> const& pieces, int cap_pct) noexcept {
    ArmorSummary s{};
    for (auto const* p : pieces) {
        if (!p) continue;
        s.slash   += p->dr_slash;
        s.pierce  += p->dr_pierce;
        s.crush   += p->dr_crush;
        s.element += p->dr_element;
        s.voice   += p->dr_voice;
    }
    if (s.slash   > cap_pct) s.slash   = cap_pct;
    if (s.pierce  > cap_pct) s.pierce  = cap_pct;
    if (s.crush   > cap_pct) s.crush   = cap_pct;
    if (s.element > cap_pct) s.element = cap_pct;
    if (s.voice   > cap_pct) s.voice   = cap_pct;
    return s;
}

void ArmorDatabase::load_from_file(char const* path) noexcept {
    if (!path) return;
    std::ifstream f(path);
    if (!f.is_open()) return;
    std::ostringstream ss;
    ss << f.rdbuf();
    try {
        load_from_json(json::parse(ss.str()));
    } catch (std::exception const& e) {
        std::fprintf(stderr, "armor.json parse error: %s\n", e.what());
    }
}

void ArmorDatabase::load_from_json(json::Value const& root) noexcept {
    if (!root.is_array()) return;
    for (auto const& v : root.as_array()) {
        if (!v.is_object()) continue;
        ArmorDef a{};
        a.code_name    = str(v.get("code"), "");
        a.display_name = str(v.get("name"), a.code_name);
        a.slot         = slot_from_string(str(v.get("slot"), "chest"));
        a.dr_slash     = num(v.get("dr_slash"),   0);
        a.dr_pierce    = num(v.get("dr_pierce"),  0);
        a.dr_crush     = num(v.get("dr_crush"),   0);
        a.dr_element   = num(v.get("dr_element"), 0);
        a.dr_voice     = num(v.get("dr_voice"),   0);
        a.encumbrance  = numf(v.get("encumbrance"), 0.0f);
        a.condition_max = num(v.get("condition_max"), 100);
        a.condition    = num(v.get("condition"), a.condition_max);
        if (auto tags_v = v.get("tags"); tags_v && tags_v->is_array()) {
            for (auto const& t : tags_v->as_array()) {
                if (t.is_string()) a.tags.push_back(t.as_string());
            }
        }
        if (auto id_v = v.get("id"); id_v && id_v->is_number()) {
            a.id = static_cast<std::uint64_t>(id_v->as_number());
        } else {
            std::uint64_t h = 0xCBF29CE484222325ULL;
            for (char c : a.code_name) {
                h ^= static_cast<std::uint64_t>(static_cast<std::uint8_t>(c));
                h *= 0x100000001B3ULL;
            }
            a.id = h;
        }
        add(std::move(a));
    }
}

void ArmorDatabase::add(ArmorDef def) noexcept {
    std::size_t const idx = order_.size();
    order_.push_back(std::move(def));
    auto const& d = order_.back();
    by_id_[d.id] = idx;
    by_code_[d.code_name] = idx;
}

ArmorDef const* ArmorDatabase::get(std::uint64_t id) const noexcept {
    auto it = by_id_.find(id);
    return it == by_id_.end() ? nullptr : &order_[it->second];
}
ArmorDef const* ArmorDatabase::get_by_code(std::string const& code) const noexcept {
    auto it = by_code_.find(code);
    return it == by_code_.end() ? nullptr : &order_[it->second];
}

}  // namespace combat
}  // namespace ash