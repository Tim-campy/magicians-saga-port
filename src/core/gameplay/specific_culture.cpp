#include "specific_culture.hpp"
#include "apply_culture.hpp"
#include "assets/culture_signs.hpp"
namespace saga::core {
namespace {
std::size_t word(std::string_view bytes,std::size_t offset) {
    if(offset>=bytes.size() || bytes.size()-offset<2)throw std::invalid_argument("truncated culture field");
    return static_cast<std::size_t>(static_cast<unsigned char>(bytes[offset]))+256U*static_cast<unsigned char>(bytes[offset+1]);
}
std::u16string text(std::string_view pool,std::size_t offset) {
    if(offset>=pool.size())throw std::invalid_argument("culture text outside pool");
    const auto end=pool.find('\0',offset);
    if(end==pool.npos)throw std::invalid_argument("unterminated culture text");
    return decode_culture_utf8(pool.substr(offset,end-offset));
}
std::shared_ptr<CultureInstance> populate(const CultureTables& tables,std::size_t row) {
    if(tables.records.size()%56 || row>=tables.records.size()/56)throw std::invalid_argument("invalid culture row");
    const auto record=tables.records.substr(row*56,56);
    auto result=std::make_shared<CultureInstance>();
    const auto lcid=word(record,0);
    result->identity.lcid=static_cast<std::int32_t>(lcid)-(lcid>=0x8000?65536:0);
    result->identity.constructed=true;
    result->name=text(tables.pool,word(record,8));
    const auto territory=word(record,0x14);
    if(territory)result->identity.territory=text(tables.pool,territory);
    const auto number=word(record,0x20);
    if(number>=0x8000)throw std::invalid_argument("negative culture number index");
    result->number_index=number;
    return result;
}
struct NameLookup:CultureNameLookup {
    explicit NameLookup(const CultureTables& value):tables(value){}
    const CultureTables& tables;
    std::optional<std::size_t> row;
    bool construct(std::u16string_view name) override {
        row=assets::find_culture_record_by_name(encode_culture_lookup_name(name),tables.names,tables.records,tables.pool);
        return row.has_value();
    }
};
}
// CultureInfo .ctor(int,bool,bool) 0x1e275c8: decoded fields needed for number parsing.
std::shared_ptr<CultureInstance> create_culture_by_lcid(std::int32_t id,bool user_override,bool read_only,
    const CultureTables& tables,SpecificCultureHost& host) {
    if(id<0)throw CultureSelectionError(CultureSelectionFailure::NegativeLcid);
    std::shared_ptr<CultureInstance> result;
    if(id==0x7f)result=host.new_invariant_culture();
    else {
        const auto row=assets::find_culture_record_by_lcid(id,tables.records);
        if(!row)throw CultureSelectionError(CultureSelectionFailure::MissingLcid);
        result=populate(tables,*row);
    }
    if(!result)throw CultureManagedError("missing new invariant culture");
    result->read_only=read_only;
    result->use_user_override=user_override;
    return result;
}
// CreateSpecificCulture 0x1e24b98 and CreateSpecificCultureFromNeutral 0x1e27fc4.
std::shared_ptr<CultureInstance> create_specific_culture(const std::optional<std::u16string>& name,
    const CultureTables& tables,SpecificCultureHost& host) {
    if(!name)throw CultureSelectionError(CultureSelectionFailure::NullName);
    if(name->empty())return host.invariant_culture();
    const auto lowered=host.lower_invariant(*name);
    NameLookup lookup(tables);
    if(!construct_culture_from_name(lowered,lookup))throw CultureSelectionError(CultureSelectionFailure::NameNotFound);
    auto result=populate(tables,*lookup.row);
    if(result->identity.lcid!=0x7f && !result->identity.territory) {
        const auto neutral_name=host.lower_invariant(result->name);
        for(const auto& entry:tables.neutral)if(entry.name==neutral_name)
            return create_culture_by_lcid(entry.lcid,true,false,tables,host);
        throw CultureSelectionError(CultureSelectionFailure::UnknownNeutral);
    }
    return result;
}
}
