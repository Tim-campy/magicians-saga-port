#include "culture_data.hpp"
#include "assets/culture_signs.hpp"
#include <fstream>
#include <limits>
#include <set>
namespace saga::desktop {
namespace {
[[noreturn]] void invalid(){throw std::invalid_argument("invalid neutral culture JSON");}
// Restricted schema reader, not a general JSON DOM. No recursion or arbitrary keys.
struct Json {
    std::string_view data;std::size_t pos=0;
    void space(){while(pos<data.size() && (data[pos]==' '||data[pos]=='\t'||data[pos]=='\n'||data[pos]=='\r'))++pos;}
    bool take(char c){space();if(pos<data.size() && data[pos]==c){++pos;return true;}return false;}
    void need(char c){if(!take(c))invalid();}
    unsigned hex(){unsigned v=0;for(int i=0;i<4;++i){if(pos==data.size())invalid();char c=data[pos++];
        unsigned d;if(c>='0'&&c<='9')d=static_cast<unsigned>(c-'0');else if(c>='a'&&c<='f')d=static_cast<unsigned>(c-'a'+10);
        else if(c>='A'&&c<='F')d=static_cast<unsigned>(c-'A'+10);else invalid();v=16*v+d;}return v;}
    std::u16string string(){
        need('"');std::u16string result;std::size_t start=pos;
        while(pos<data.size()){
            auto c=static_cast<unsigned char>(data[pos]);
            if(c=='"'||c=='\\'){
                result+=core::decode_culture_utf8(data.substr(start,pos-start));++pos;
                if(c=='"')return result;
                if(pos==data.size())invalid();char escape=data[pos++];
                switch(escape){
                case '"':result+=u'"';break;case '\\':result+=u'\\';break;case '/':result+=u'/';break;
                case 'b':result+=u'\b';break;case 'f':result+=u'\f';break;case 'n':result+=u'\n';break;
                case 'r':result+=u'\r';break;case 't':result+=u'\t';break;
                case 'u':result+=static_cast<char16_t>(hex());break;default:invalid();}
                start=pos;
            }else {if(c<32)invalid();++pos;}
        }invalid();
    }
    std::int32_t integer(){
        space();bool negative=take('-');
        if(pos==data.size()||data[pos]<'0'||data[pos]>'9')invalid();
        const bool zero=data[pos]=='0';std::uint64_t v=0;std::size_t digits=0;
        while(pos<data.size()&&data[pos]>='0'&&data[pos]<='9'){
            if(zero&&digits)invalid();++digits;v=v*10+static_cast<unsigned>(data[pos++]-'0');
            if(v>(negative?2147483648ULL:2147483647ULL))invalid();
        }
        return negative?static_cast<std::int32_t>(-static_cast<std::int64_t>(v)):static_cast<std::int32_t>(v);
    }
    core::NeutralCultureEntry entry(){
        need('{');core::NeutralCultureEntry value;bool name=false,id=false;
        do{auto key=string();need(':');
            if(key==u"name"&&!name){value.name=string();name=true;}
            else if(key==u"lcid"&&!id){value.lcid=integer();id=true;}else invalid();
        }while(take(','));need('}');if(!name||!id)invalid();return value;
    }
};
std::string read(const std::filesystem::path& path,std::size_t limit){
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file)throw std::runtime_error("cannot open culture data: "+path.string());
    auto length=file.tellg();if(length<0||static_cast<std::uintmax_t>(length)>limit)throw std::invalid_argument("culture file extent");
    std::string result(static_cast<std::size_t>(length),'\0');file.seekg(0);
    if(!file.read(result.data(),static_cast<std::streamsize>(result.size())))throw std::runtime_error("cannot read culture data");
    return result;
}
std::size_t word(std::string_view b,std::size_t o){return static_cast<unsigned char>(b[o])+256U*static_cast<unsigned char>(b[o+1]);}
std::string_view text(std::string_view pool,std::size_t offset){
    if(offset>=pool.size())throw std::invalid_argument("culture pool offset");
    auto end=pool.find('\0',offset);if(end==pool.npos)throw std::invalid_argument("culture pool terminator");
    auto value=pool.substr(offset,end-offset);core::decode_culture_utf8(value);return value;
}
}
std::vector<core::NeutralCultureEntry> parse_neutral_cultures(std::string_view input){
    if(input.size()>1024*1024)invalid();Json json{input};json.need('{');bool schema=false,entries=false;
    std::vector<core::NeutralCultureEntry> result;
    do{auto key=json.string();json.need(':');
        if(key==u"schema"&&!schema){if(json.integer()!=1)invalid();schema=true;}
        else if(key==u"entries"&&!entries){entries=true;json.need('[');
            if(!json.take(']')){do{result.push_back(json.entry());}while(json.take(','));json.need(']');}}
        else invalid();
    }while(json.take(','));json.need('}');json.space();if(!schema||!entries||json.pos!=input.size())invalid();
    std::set<std::u16string> seen;
    for(const auto& value:result)if(!seen.insert(value.name).second)invalid();
    return result;
}
core::CultureLowerTables CultureData::lowercase_tables()const{
    core::CultureLowerTables result;for(std::size_t i=0;i<9;++i)result[i]=lower_[i];return result;
}
CultureData CultureData::load(const std::filesystem::path& root){
    CultureData result;const auto dir=root/"culture";
    result.names_=read(dir/"name-index.bin",339*4);result.records_=read(dir/"culture-records.bin",339*56);
    result.numbers_=read(dir/"number-records.bin",339*48);result.pool_=read(dir/"sign-pool.bin",65536);
    if(result.names_.size()!=339*4||result.records_.size()!=339*56||result.numbers_.size()!=339*48)
        throw std::invalid_argument("culture table counts");
    std::set<std::size_t> rows,numbers;std::string_view previous;int prior_id=std::numeric_limits<int>::min();
    for(std::size_t i=0;i<339;++i){
        const auto name=text(result.pool_,word(result.names_,i*4));
        if(i && name<=previous)throw std::invalid_argument("culture names must be unique and sorted");previous=name;
        auto row=word(result.names_,i*4+2);if(row>=339||!rows.insert(row).second)throw std::invalid_argument("culture row permutation");
        auto number=word(result.records_,i*56+0x20);if(number>=339||!numbers.insert(number).second)throw std::invalid_argument("number index permutation");
        text(result.pool_,word(result.records_,i*56+8));auto territory=word(result.records_,i*56+0x14);
        if(territory)text(result.pool_,territory);
        auto raw=word(result.records_,i*56);int id=static_cast<int>(raw)-(raw>=32768?65536:0);
        if(id<prior_id)throw std::invalid_argument("culture LCIDs must be sorted");prior_id=id;
        core::read_apply_number_signs(result.numbers_,result.pool_,i);
    }
    constexpr std::array<std::size_t,9> lengths{2350,76,1018,32,52,94,262,174,212};
    for(std::size_t i=0;i<9;++i){result.lower_[i]=read(dir/("lower-"+std::to_string(i)+".bin"),lengths[i]);
        if(result.lower_[i].size()!=lengths[i])throw std::invalid_argument("lowercase table extent");}
    result.neutral_=parse_neutral_cultures(read(dir/"neutral-map.json",1024*1024));
    if(result.neutral_.size()!=130)throw std::invalid_argument("neutral mapping count");
    // Do not reject absent target LCIDs: shipped neutral dispatch includes failing cases.
    return result;
}
std::u16string CultureDataHost::lower_invariant(const std::u16string& value){return core::lower_invariant_text(value,data_.lowercase_tables());}
std::shared_ptr<core::CultureInstance> CultureDataHost::specific(const std::optional<std::u16string>& name){return core::create_specific_culture(name,data_.tables(),*this);}
bool CultureDataHost::populate(core::CultureInstance& culture,std::int32_t lcid){return core::populate_culture_by_lcid(culture,lcid,data_.tables());}
std::shared_ptr<core::CultureNumberFormat> CultureDataHost::construct_number_format(const core::CultureInstance& culture){
    return core::read_culture_number_format(culture,false,data_.number_records(),data_.tables().pool);
}
std::shared_ptr<core::CultureNumberFormat> CultureDataHost::get_format(core::CultureInstance& culture){
    if(culture.inherited)throw std::logic_error("derived culture GetFormat requires its own host adapter");
    return core::culture_number_format(culture,*this);
}
}
