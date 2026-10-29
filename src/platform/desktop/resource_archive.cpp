#include "resource_archive.hpp"
#include "assets/jar_archive.hpp"
#include "core/gameplay/resource_lookup.hpp"
#include "core/gameplay/animation_resources.hpp"
#include "core/gameplay/event_catalog.hpp"
#include "core/gameplay/presentation_text.hpp"
#include <locale>
#include <map>
#include <stdexcept>
#include <utility>

namespace saga::desktop {
namespace {
using namespace core;
CatalogText wide(std::string_view s) {
    std::u16string result;
    result.reserve(s.size());
    for (const char raw : s) {
        const auto c = static_cast<unsigned char>(raw);
        if (c > 127) throw std::invalid_argument("archive resource profile requires ASCII text");
        result.push_back(static_cast<char16_t>(c));
    }
    return std::make_shared<const std::u16string>(std::move(result));
}
std::string narrow(std::u16string_view s) {
    std::string result;
    result.reserve(s.size());
    for (const auto c : s) {
        if (c > 127) throw std::invalid_argument("archive resource profile requires ASCII text");
        result.push_back(static_cast<char>(c));
    }
    return result;
}
std::string lower(std::string s) {
    for (auto& c : s) c = std::tolower(c, std::locale::classic());
    return s;
}
} // namespace

struct ResourceArchive::Impl final : JarResourceHost, ResourceNameHost, ResourceDictionary {
    std::vector<std::uint8_t> bytes;
    assets::JarArchive archive;
    ResourceProfile profile;
    ResourceLookupState state;
    JarResourceLookup jar{this};
    std::map<std::u16string,std::size_t> members;
    ArchiveCatalog image_catalog, animation_catalog;

    Impl(std::vector<std::uint8_t> data, ResourceProfile selected)
        : bytes(std::move(data)), archive(assets::parse_jar(bytes.data(),bytes.size())), profile(std::move(selected)) {
        state.country = wide(""); // Explicit prepared locale; never invoke Android initialization.
        state.dpi = profile.dpi;
        wide(profile.language_folder); // Validate the supported profile before publication.
        for (std::size_t i=0;i<archive.members.size();++i) {
            const auto key = lower_case(*wide(archive.members[i].name));
            if (!members.emplace(*key,i).second) throw std::invalid_argument("case-folded archive member collision");
        }
        image_catalog = catalog("img.inf");
        animation_catalog = catalog("seb.inf");
    }
    void initialize(ResourceLookupState&) override { throw std::logic_error("unbound Android resource initialization"); }
    std::int32_t extension_selector() override { return 2; } // Shipped Config initial selector.
    bool use_resource_language() override { return true; } // No language-pack override in this profile.
    CatalogText language_folder() override { return wide(profile.language_folder); }
    CatalogText trim(const std::u16string& value) override {
        auto s = narrow(value);
        std::size_t first=0,last=s.size();
        while (first<last && std::isspace(s[first],std::locale::classic())) ++first;
        while (last>first && std::isspace(s[last-1],std::locale::classic())) --last;
        return wide(std::string_view(s).substr(first,last-first));
    }
    CatalogText lower_case(const std::u16string& s) override { return wide(lower(narrow(s))); }
    bool starts_with(const std::u16string& s,std::u16string_view prefix) override {
        const auto input=narrow(s), start=narrow(prefix); // Explicit ASCII/invariant profile.
        return input.compare(0,start.size(),start)==0;
    }
    CatalogText convert_jar_extension(CatalogText s) override { return convert_resource_extension(std::move(s),2); }
    std::int32_t parse_id(std::u16string_view s) override { return parse_event_decimal(narrow(s)); }
    std::optional<std::size_t> find(const std::u16string& key) override {
        const auto it=members.find(key);
        return it==members.end()?std::nullopt:std::optional<std::size_t>(it->second);
    }
    std::optional<std::size_t> lookup(std::string_view name) {
        return search_resource_member(jar,state,wide(name),*this);
    }
    std::vector<std::uint8_t> payload(std::size_t index) const {
        const auto& m=archive.members.at(index);
        return {bytes.begin()+static_cast<std::ptrdiff_t>(m.payload_offset),
                bytes.begin()+static_cast<std::ptrdiff_t>(m.payload_offset+m.size)};
    }
    ArchiveCatalog catalog(const char* name) {
        const auto id=lookup(name);
        if (!id) throw std::invalid_argument(std::string("missing archive catalog: ")+name);
        const auto data=payload(*id);
        std::string_view input(reinterpret_cast<const char*>(data.data()),data.size());
        if (input.substr(0,3)=="\xef\xbb\xbf") input.remove_prefix(3);
        auto rows=std::make_shared<std::vector<CatalogText>>();
        while (!input.empty()) {
            const auto end=input.find('\n');
            const auto line=input.substr(0,end);
            if (!line.empty()) rows->push_back(wide(line)); // Unity 2022 profile: retain CR.
            if (end==input.npos) break;
            input.remove_prefix(end+1);
        }
        trim_resource_catalog(rows,*this);
        const auto ids=parse_resource_catalog_rows(rows,*this);
        const auto indexed=index_resource_catalog(ids);
        ArchiveCatalog result{std::string(archive.members.at(*id).name),{}};
        result.slots.resize(indexed.rows.size());
        for (std::size_t i=0;i<indexed.rows.size();++i)
            if (indexed.rows[i]) result.slots[i]=narrow(*rows->at(*indexed.rows[i]));
        return result;
    }
    std::string specification(const ArchiveCatalog& catalog,std::int32_t slot) {
        const auto& value=catalog.slots.at(static_cast<std::size_t>(slot));
        if (!value) throw std::invalid_argument("null archive resource slot");
        return narrow(*resource_file_name(wide(*value),*this));
    }
    ArchiveSprite sprite(std::int32_t clip,std::int32_t frame,std::int32_t layer) {
        const auto spec=wide(specification(animation_catalog,clip));
        const auto parts=split_trimmed_text_fields(spec.get(),u',');
        if (parts.size()!=1) throw std::invalid_argument("unbound SEB loading option");
        const auto seb_id=lookup(narrow(parts.at(0)));
        if (!seb_id) throw std::invalid_argument("SEB member unavailable in selected archive");
        const auto seb_bytes=payload(*seb_id);
        ArchiveSprite result;
        result.animation=assets::parse_seb(seb_bytes.data(),seb_bytes.size());
        auto prepared=prepare_seb_sprites(result.animation); SebStaticWork work;
        const auto value=seb_sprite(prepared,work,frame,layer);
        if (!value || value->at(1)==-1) throw std::invalid_argument("selected sprite has no image");
        result.texture=value->at(1);
        const auto image_spec=wide(specification(image_catalog,result.texture));
        const auto image_parts=split_trimmed_text_fields(image_spec.get(),u',');
        for (std::size_t i=1;i<image_parts.size();++i) {
            const auto option=lower(narrow(image_parts[i]));
            if (option=="bin") continue;
            const auto x=option.find('x');
            if (x==option.npos) throw std::invalid_argument("unbound image loading option: "+option);
            result.standard_width=parse_event_decimal(std::string_view(option).substr(0,x));
            result.standard_height=parse_event_decimal(std::string_view(option).substr(x+1));
        }
        const auto image_id=lookup(narrow(image_parts.at(0)));
        if (!image_id) throw std::invalid_argument("PNG member unavailable in selected archive");
        result.png=payload(*image_id);
        result.image_member=archive.members.at(*image_id).name;
        result.animation_member=archive.members.at(*seb_id).name;
        result.image_catalog=image_catalog.member;
        result.animation_catalog=animation_catalog.member;
        const auto opt_name=image_option_file(image_spec);
        if (const auto opt_id=lookup(narrow(*opt_name))) {
            const auto data=payload(*opt_id);
            result.opt=assets::parse_opt(data.data(),data.size());
            result.opt_member=archive.members.at(*opt_id).name;
        }
        return result;
    }
};

ResourceArchive::ResourceArchive(std::vector<std::uint8_t> bytes,ResourceProfile profile)
    : impl_(std::make_unique<Impl>(std::move(bytes),std::move(profile))) {}
ResourceArchive::~ResourceArchive()=default;
const ArchiveCatalog& ResourceArchive::images() const { return impl_->image_catalog; }
const ArchiveCatalog& ResourceArchive::animations() const { return impl_->animation_catalog; }
ArchiveSprite ResourceArchive::sprite(std::int32_t clip,std::int32_t frame,std::int32_t layer) { return impl_->sprite(clip,frame,layer); }
} // namespace saga::desktop
