#include "core/gameplay/resource_lookup.hpp"
#include "core/gameplay/animation_resources.hpp"

#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>

using namespace saga::core;
namespace {
void check_at(bool value, int line) {
    if (!value) { std::cerr << "resource lookup contract line " << line << '\n'; std::abort(); }
}
#define check(...) check_at((__VA_ARGS__), __LINE__)
template<class E, class F> void fails(F operation) {
    bool caught = false;
    try { operation(); } catch (const E&) { caught = true; }
    check(caught);
}
CatalogText text(const char16_t* value) { return std::make_shared<const std::u16string>(value); }
CatalogRows rows(std::initializer_list<CatalogText> values) {
    return std::make_shared<std::vector<CatalogText>>(values);
}
std::vector<std::u16string> values(const CatalogRows& array) {
    std::vector<std::u16string> result;
    for (const auto& row : *array) result.push_back(*row);
    return result;
}
struct Dictionary : ResourceDictionary {
    std::map<std::u16string, std::size_t> entries;
    std::vector<std::u16string> queries;
    std::optional<std::size_t> find(const std::u16string& key) override {
        queries.push_back(key);
        const auto it = entries.find(key);
        return it == entries.end() ? std::nullopt : std::optional<std::size_t>(it->second);
    }
};
struct Host : JarResourceHost, CatalogTextHost {
    bool use = true;
    std::int32_t selector = 2;
    CatalogText folder = text(u"English.lproj");
    std::vector<std::string> calls;
    std::function<void(const std::string&)> hook;
    void step(const std::string& name) { calls.push_back(name); if (hook) hook(name); }
    void initialize(ResourceLookupState& state) override { step("init"); state.country = text(u""); }
    std::int32_t extension_selector() override { step("selector"); return selector; }
    bool use_resource_language() override { step("use"); return use; }
    CatalogText language_folder() override { step("folder"); return folder; }
    CatalogText trim(const std::u16string& s) override {
        step("trim");
        const auto first = s.find_first_not_of(u" \t\r\n");
        if (first == s.npos) return text(u"");
        const auto last = s.find_last_not_of(u" \t\r\n");
        return std::make_shared<const std::u16string>(s.substr(first, last - first + 1));
    }
    CatalogText lower_case(const std::u16string& s) override {
        step("lower");
        auto result = s;
        for (auto& ch : result) if (ch >= u'A' && ch <= u'Z') ch = static_cast<char16_t>(ch + (u'a' - u'A'));
        return std::make_shared<const std::u16string>(std::move(result));
    }
    CatalogText convert_jar_extension(CatalogText s) override {
        step("jar");
        return convert_resource_extension(std::move(s), extension_selector()); // Non-prefixed fixture jar.
    }
    std::int32_t parse_id(std::u16string_view s) override {
        std::string ascii;
        for (const auto ch : s) {
            if (ch > 127) throw std::invalid_argument("non-ASCII fixture ID");
            ascii.push_back(static_cast<char>(ch));
        }
        return std::stoi(ascii);
    }
    // Trim/lower/parse are synthetic ASCII fixture services, not production
    // Unicode/culture replacements. The core keeps those contracts external.
};
ResourceLookupState state_without_dpi() {
    ResourceLookupState result;
    result.country = text(u"");
    result.dpi_folders = rows({});
    return result;
}
}

int main() {
    {const auto name = text(u"a.gif/b.gif.mld.GIF");
     for (const int mode : {2,3,4}) check(*convert_resource_extension(name,mode) == u"a.png/b.png.ogg.GIF");
     for (const int mode : {-1,0,1,5}) check(convert_resource_extension(name,mode) == name);
     check(convert_resource_extension(text(u"x.png"),2) != nullptr);
     check(!convert_resource_extension(nullptr,2));}
    {Host host; auto state = state_without_dpi(); state.dpi_folders = rows({text(u"lo"),text(u"mid"),text(u"hi")});
     state.dpi = 1; state.language_pack_folders = rows({text(u"packA"),text(u"packB")});
     const auto result = resource_access_files(state,text(u" root / file.gif "),host);
     check(values(result) == std::vector<std::u16string>{
         u"root/mid/packA/file.png",u"root/mid/packB/file.png",u"root/mid/English.lproj/file.png",u"root/mid/file.png",
         u"root/hi/packA/file.png",u"root/hi/packB/file.png",u"root/hi/English.lproj/file.png",u"root/hi/file.png",
         u"root/lo/packA/file.png",u"root/lo/packB/file.png",u"root/lo/English.lproj/file.png",u"root/lo/file.png",
         u"root/packA/file.png",u"root/packB/file.png",u"root/English.lproj/file.png",u"root/file.png"});
     check(host.calls == std::vector<std::string>{"selector","use","folder"});}
    {Host host; auto state = state_without_dpi(); state.language_pack_folders = rows({text(u"pack")}); host.use = false;
     check(values(resource_access_files(state,text(u"img.inf"),host)) == std::vector<std::u16string>{u"English.lproj/img.inf",u"img.inf"});
     check(host.calls == std::vector<std::string>{"selector","use","folder"});
     host.calls.clear(); state.language_pack_folders.reset();
     resource_access_files(state,text(u"img.inf"),host);
     check(host.calls == std::vector<std::string>{"selector","folder"});
     state.language_pack_folders = rows({}); host.calls.clear();
     resource_access_files(state,text(u"img.inf"),host);
     check(host.calls == std::vector<std::string>{"selector","use","folder"});}
    {Host host; auto state = state_without_dpi(); state.language_pack_folders = rows({text(u""),nullptr,text(u"English.lproj")});
     check(values(resource_access_files(state,text(u"d//f"),host)) == std::vector<std::u16string>{
         u"d///f",u"d///f",u"d//English.lproj/f",u"d//English.lproj/f",u"d//f"});
     host.folder.reset(); state.language_pack_folders.reset();
     check(values(resource_access_files(state,text(u"d\\f"),host)) == std::vector<std::u16string>{u"/d\\f",u"d\\f"});
     check(values(resource_access_files(state,text(u"\u00a0/f/"),host)) == std::vector<std::u16string>{u"\u00a0/f//",u"\u00a0/f/"});}
    {Host host; ResourceLookupState state;
     const auto result = resource_access_files(state,text(u"file"),host);
     check(result->size() == 10 && *result->at(0) == u"drawable-ldpi/English.lproj/file");
     check(host.calls.front() == "init" && state.country && state.country->empty());
     host.calls.clear(); resource_access_files(state,text(u"file"),host); check(host.calls.front() == "selector");
     state.dpi = 4; check(*resource_access_files(state,text(u"file"),host)->at(0) == u"drawable-xhdpi/English.lproj/file");
     state.dpi = -1; fails<std::out_of_range>([&]{resource_access_files(state,text(u"file"),host);});
     state.dpi = 5; fails<std::out_of_range>([&]{resource_access_files(state,text(u"file"),host);});}
    {Host host; auto state = state_without_dpi();
     fails<std::out_of_range>([&]{resource_access_files(state,nullptr,host);});
     fails<std::out_of_range>([&]{resource_access_files(state,text(u""),host);});
     state.language_pack_folders = rows({text(u"pack")});
     host.hook = [&](const std::string& call){if(call == "use")state.language_pack_folders.reset();};
     fails<std::invalid_argument>([&]{resource_access_files(state,text(u"file"),host);});
     check(host.calls.back() == "use");}
    {Host host; auto state = state_without_dpi(); state.dpi_folders = rows({text(u"lo"),text(u"hi")}); state.dpi = 1;
     Dictionary dictionary; dictionary.entries={{u"hi/english.lproj/file.png",0},{u"file.png",9}};
     JarResourceLookup jar{&dictionary};
     // The first extension conversion preserves uppercase .GIF. Lower casing
     // exposes .gif to the second conversion inside GetAccessFilesList.
     check(search_resource_member(jar,state,text(u" FILE.GIF "),host) == 0);
     check(dictionary.queries == std::vector<std::u16string>{u"hi/english.lproj/file.png"});
     check(host.calls == std::vector<std::string>{"trim","jar","selector","lower","selector","folder","lower"});
     host.calls.clear(); dictionary.queries.clear();
     check(!search_resource_member(jar,state,nullptr,host) && host.calls.empty());
     jar.dictionary = nullptr; check(!search_resource_member(jar,state,text(u"f"),host) && host.calls.empty());}
    {Host host; auto state = state_without_dpi(); Dictionary first,second; second.entries={{u"file",7}};
     JarResourceLookup jar{&first}; int lowers = 0;
     host.hook = [&](const std::string& call){if(call == "lower" && ++lowers == 2)jar.dictionary=&second;};
     check(search_resource_member(jar,state,text(u"file"),host) == 7);
     check(first.queries == std::vector<std::u16string>{u"english.lproj/file"});
     check(second.queries == std::vector<std::u16string>{u"file"});}
    {Host host; auto state = state_without_dpi(); state.language_pack_folders = rows({text(u"custom")});
     Dictionary dictionary; dictionary.entries={{u"custom/img.inf",0},{u"english.lproj/img.inf",1}};
     JarResourceLookup jar{&dictionary};
     const std::vector<CatalogRows> contents{rows({text(u"2\tcustom.png,bin")}),rows({text(u"0\tbase.png,bin"),text(u"2\tbase2.png,bin")})};
     auto selected = contents.at(*search_resource_member(jar,state,text(u"img.inf"),host));
     host.use = false;
     const auto base = contents.at(*search_resource_member(jar,state,text(u"img.inf"),host));
     trim_resource_catalog(selected,host); merge_resource_catalog(selected,base,host);
     check(values(selected) == std::vector<std::u16string>{u"2\tcustom.png,bin",u"0\tbase.png,bin"});
     const auto indexed = index_resource_catalog({2,0});
     check(indexed.rows.size() == 3 && indexed.rows[0] == 1 && indexed.rows[2] == 0 && !indexed.rows[1]);}
    std::cout << "resource lookup contract ok\n";
}
