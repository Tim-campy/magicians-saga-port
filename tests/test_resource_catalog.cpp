#include "core/gameplay/resource_catalog.hpp"
#include "core/gameplay/animation_resources.hpp"

#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace saga::core;
namespace {
void check(bool value) { if (!value) std::abort(); }
template<class E, class F> void fails(F operation) {
    bool caught = false;
    try { operation(); } catch (const E&) { caught = true; }
    check(caught);
}
CatalogText text(const char16_t* value) { return std::make_shared<const std::u16string>(value); }
CatalogRows rows(std::initializer_list<CatalogText> values) {
    return std::make_shared<std::vector<CatalogText>>(values);
}
struct Host final : ResourceNameHost {
    int trims = 0, parses = 0;
    bool starts_with(const std::u16string& s,std::u16string_view prefix) override {
        return s.compare(0,prefix.size(),prefix)==0; // Synthetic ASCII fixture provider.
    }
    CatalogText trim(const std::u16string& value) override {
        ++trims;
        // Synthetic ASCII fixture service, not a Unicode/runtime replacement.
        const auto first = value.find_first_not_of(u" \t\r\n");
        if (first == value.npos) return text(u"");
        const auto last = value.find_last_not_of(u" \t\r\n");
        return std::make_shared<const std::u16string>(value.substr(first, last - first + 1));
    }
    std::int32_t parse_id(std::u16string_view value) override {
        ++parses;
        std::string ascii;
        ascii.reserve(value.size());
        for (const auto ch : value) {
            if (ch > 127) throw std::invalid_argument("non-ASCII fixture ID");
            ascii.push_back(static_cast<char>(ch));
        }
        std::size_t count = 0;
        const auto result = std::stoi(ascii, &count);
        if (count != ascii.size()) throw std::invalid_argument("fixture ID");
        return result;
    }
};
}

int main() {
    {Host host;
     auto options=seb_load_options(text(u" clip.seb ,u-any,v,c3,c0,c2,Z,U"),host);
     check(*options.filename==u"clip.seb"&&options.reverse_u==1&&options.reverse_v==1&&options.cache_size==2);
     options=seb_load_options(text(u"clip.seb,U,V,c-2"),host);
     check(options.reverse_u==0&&options.reverse_v==0&&options.cache_size==-2);
     fails<std::out_of_range>([&]{seb_load_options(text(u"clip.seb,"),host);});}
    {Host host; auto chosen = rows({text(u" 2\t file.png,bin \tignored"),text(u"plain.seb"),text(u"")});
     check(parse_resource_catalog_rows(chosen,host) == std::vector<std::int32_t>({2,-1,-1}));
     check(*chosen->at(0) == u"file.png,bin" && *chosen->at(1) == u"plain.seb");
     chosen = rows({text(u"1\tfirst"),text(u"bad\tfault")});
     fails<std::invalid_argument>([&]{parse_resource_catalog_rows(chosen,host);});
     check(*chosen->at(0) == u"first" && *chosen->at(1) == u"bad\tfault");}
    {Host host;
     check(*resource_file_name(text(u"../a/b.png,bin "),host) == u"b.png,bin");
     check(*resource_file_name(text(u"..\\a/b\\c.png "),host) == u"c.png");
     check(*resource_file_name(text(u"a/b.png "),host) == u"a/b.png");
     check(*resource_file_name(text(u" ../a/b.png "),host) == u"../a/b.png");
     fails<std::invalid_argument>([&]{resource_file_name(nullptr,host);});
     check(*image_option_file(text(u"chip.png,bin")) == u"chip.opt");
     check(*image_option_file(text(u"dir.with-dot/file")) == u"dir.opt");
     check(*image_option_file(text(u".png")) == u".opt");
     check(!image_option_file(nullptr));
     fails<std::out_of_range>([]{image_option_file(text(u"no-dot,bin"));});}
    {Host host; auto chosen = rows({text(u" 2\tselected.png,bin "), text(u"3\tchosen3")});
     auto alias = chosen;
     trim_resource_catalog(chosen, host);
     check(*chosen->at(0) == u"2\tselected.png,bin" && chosen == alias);
     merge_resource_catalog(chosen, rows({text(u"0\tbase0"),text(u"2\tbase2"),text(u"4\tbase4")}), host);
     check(chosen != alias && alias->size() == 2 && chosen->size() == 4);
     check(*chosen->at(2) == u"0\tbase0" && *chosen->at(3) == u"4\tbase4");
     std::vector<std::int32_t> ids;
     for (const auto& row : *chosen) ids.push_back(host.parse_id(std::u16string_view(*row).substr(0,row->find(u'\t'))));
     const auto indexed = index_resource_catalog(ids);
     check(indexed.rows.size() == 5 && indexed.rows[2] == 0 && indexed.rows[0] == 2 && indexed.rows[4] == 3);
     check(!indexed.rows[1]);}
    {Host host; auto chosen = rows({nullptr,text(u""),text(u"2\tselected2"),text(u"implicit3")});
     merge_resource_catalog(chosen, rows({nullptr,text(u"  "),text(u"implicit0"),text(u"implicit1"),
                                        text(u"2\tbase2"),text(u"implicit3"),text(u"4\tbase4"),text(u"4\tduplicate4")}), host);
     check(chosen->size() == 8);
     check(*chosen->at(4) == u"implicit0" && *chosen->at(5) == u"implicit1");
     check(*chosen->at(6) == u"4\tbase4" && *chosen->at(7) == u"4\tduplicate4");
     check(host.trims == 13); // Null skips; blank trims once; nonempty base rows twice.
     const auto indexed = index_resource_catalog({2,-1,4,4});
     check(indexed.ids == std::vector<std::int32_t>({2,0,4,4}) && indexed.rows[4] == 3);}
    {Host host; auto chosen = rows({text(u"0\tselected")}); auto alias = chosen;
     merge_resource_catalog(chosen, rows({text(u"0\tignored")}), host); check(chosen == alias);
     merge_resource_catalog(chosen, rows({text(u"1\tfirst"),text(u"1\tlast")}), host);
     check(chosen->size() == 3 && *chosen->at(2) == u"1\tlast");}
    {Host host; auto chosen = rows({text(u"256\tselected")});
     merge_resource_catalog(chosen, rows({text(u"255\tbase"),text(u"256\tignored"),text(u"257\tbase")}), host);
     check(chosen->size() == 3);
     fails<std::length_error>([&]{merge_resource_catalog(chosen, rows({}), host, 256);});}
    {Host host; auto chosen = rows({text(u"0\tselected")}); auto original = chosen;
     fails<std::invalid_argument>([&]{merge_resource_catalog(chosen, rows({text(u"1\tkept"),text(u"bad\tfault")}), host);});
     check(chosen != original && chosen->size() == 2 && original->size() == 1);
     fails<std::out_of_range>([&]{merge_resource_catalog(chosen, rows({text(u"-1\tfault")}), host);});
     check(chosen->size() == 2);}
    {Host host; auto chosen = rows({text(u"-1\tfault")});
     fails<std::out_of_range>([&]{merge_resource_catalog(chosen, rows({}), host);});
     chosen = rows({text(u"2147483647\tfault")});
     fails<std::length_error>([&]{merge_resource_catalog(chosen, rows({}), host);});}
    {Host host; auto chosen = rows({text(u"0\tfirst"),nullptr,text(u"2\tlast")});
     fails<std::invalid_argument>([&]{trim_resource_catalog(chosen, host);});
     check(host.trims == 1 && *chosen->at(2) == u"2\tlast");
     CatalogRows missing; merge_resource_catalog(missing, nullptr, host);
     fails<std::invalid_argument>([&]{merge_resource_catalog(missing, rows({}), host);});}
    std::cout << "resource catalog merge contract ok\n";
}
