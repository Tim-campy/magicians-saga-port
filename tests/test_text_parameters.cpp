#include "core/gameplay/text_parameters.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace saga::core;
void check(bool ok, const char* label) {
    if (!ok) { std::cerr << label << '\n'; std::exit(1); }
}
struct Host : TextParameterHost {
    std::vector<std::u16string> calls;
    bool native_failure = false;
    // Explicit fixture values, not a replacement production numeric parser.
    std::int32_t parse_decimal(const std::u16string& value) override {
        calls.push_back(u"D:"+value);
        if (native_failure) throw std::logic_error("native failure");
        if (value == u"1") return 1;
        if (value == u"2") return 2;
        if (value == u"3") return 3;
        if (value == u"256") return 256;
        if (value == u"-1") return -1;
        if (value == u"0") return 0;
        if (value == u"10") return 10;
        throw ManagedTextParseError("fixture decimal failure");
    }
    std::int32_t parse_hexadecimal(const std::u16string& value) override {
        calls.push_back(u"H:"+value);
        if (value == u"4a86e8") return 0x4a86e8;
        if (value == u"10") return 16;
        if (value == u"0") return 0;
        throw ManagedTextParseError("fixture hex failure");
    }
};
struct ScanPart {
    std::int32_t begin,end,index;
    std::u16string text,parameter;
    bool has_parameter;
};
struct Scanner : TextMarkupHost {
    std::vector<ScanPart> parts;
    std::vector<std::u16string> comparisons;
    bool fail_compare=false;
    bool equal_tag(std::u16string_view expected,std::u16string_view candidate)override{
        comparisons.push_back(u"E:"+std::u16string(expected));
        if(fail_compare)throw std::logic_error("comparison provider");
        return expected==candidate; // Explicit ordinal fixture, not product culture selection.
    }
    bool starts_with_tag(std::u16string_view candidate,std::u16string_view prefix)override{
        comparisons.push_back(u"P:"+std::u16string(prefix));
        return candidate.substr(0,prefix.size())==prefix;
    }
    void retain(const TextMarkupSpan& span){parts.push_back({span.source_begin,span.source_end,span.tag_index,std::u16string(span.text),std::u16string(span.parameter),span.has_parameter});}
    void text(const TextMarkupSpan& span)override{retain(span);}
    void tag(const TextMarkupSpan& span)override{retain(span);}
    std::u16string ordinary()const{std::u16string result;for(const auto& p:parts)if(p.index<0)result+=p.text;return result;}
};
void scanner_contracts(){
    {Scanner h;scan_text_markup(u"A\n<co=10>B</co>C",h);
     check(h.parts.size()==5&&h.ordinary()==u"A\nBC","preprocessed LF is ordinary text; tags retain order");
     check(h.parts[1].index==0&&h.parts[1].begin==2&&h.parts[1].end==9&&h.parts[1].parameter==u"10"&&h.parts[1].has_parameter,"parameter tag source span");
     check(h.parts[3].index==1&&!h.parts[3].has_parameter&&h.parts[3].begin==10&&h.parts[3].end==15,"exact end tag source span");
     check(h.comparisons[0]==u"E:<co=>"&&h.comparisons[1]==u"P:<co=","exact match precedes parameter-prefix query");}
    {Scanner h;scan_text_markup(u"<bad<co=1>>",h);
     check(h.parts.size()==3&&h.parts[0].text==u"<bad"&&h.parts[1].index==0&&h.parts[1].begin==4&&h.parts[2].text==u">","unknown opener is literal but does not swallow a nested recognized tag");}
    {Scanner h;scan_text_markup(u"x<bad <tail",h);check(h.parts.size()==1&&h.ordinary()==u"x<bad <tail","unterminated markup remains literal");
     Scanner br;scan_text_markup(u"a<br>b",br);check(br.ordinary()==u"a<br>b","literal br replacement belongs to pre-scan caller");}
    {const auto source=u"a<"+std::u16string(32,u'x')+u">z";Scanner h;scan_text_markup(source,h);
     check(h.parts.size()==2&&h.parts[0].begin==0&&h.parts[0].end==1&&h.parts[1].begin==2&&h.parts[1].end==36,"overlong opener creates source gap");
     check(h.ordinary()==u"a"+std::u16string(32,u'x')+u">z","overlong candidate drops only opening unit");
     Scanner edge;scan_text_markup(u"<co="+std::u16string(28,u'1')+u">",edge);check(edge.parts.size()==1&&edge.parts[0].index==0&&edge.parts[0].parameter.size()==28,"closing distance32 is accepted");
     Scanner over;scan_text_markup(u"<co="+std::u16string(29,u'1')+u">",over);check(over.parts.size()==1&&over.parts[0].index==-1&&over.parts[0].begin==1,"closing distance33 drops opener before tag matching");}
    {Scanner h;scan_text_markup(u"\U0001f642<co=>\u03a9</co>",h);
     check(h.parts[0].end==2&&h.parts[1].begin==2&&h.parts[1].end==7&&h.parts[1].has_parameter&&h.parts[1].parameter.empty(),"UTF16 indices count surrogate units and retain empty parameter");
     check(h.parts[2].begin==7&&h.parts[2].end==8,"non-ASCII ordinary units preserved");}
    {Scanner h;scan_text_markup(u"<co=></co><si=1></si><st=1></st><bl=1></bl><po=cm><pic=1><bo=1></bo><btn=1><ff><kr=1><em></em><ruby=1></ruby>",h);
     check(h.parts.size()==19,"all native tag templates recognized");for(std::size_t i=0;i<h.parts.size();++i)check(h.parts[i].index==static_cast<std::int32_t>(i),"native TAG index order");}
    {Scanner h;h.fail_compare=true;bool failed=false;try{scan_text_markup(u"A<co=1>",h);}catch(const std::logic_error&){failed=true;}
     check(failed&&h.parts.size()==1&&h.parts[0].text==u"A","comparison failure retains already emitted ordinary prefix");}
    {struct NumericScanner final:Scanner{Host numeric;TextTagNumbers numbers{};void tag(const TextMarkupSpan& s)override{Scanner::tag(s);if(s.has_parameter)numbers=parse_text_tag_numbers(s.tag_index,std::u16string(s.parameter),numeric);}}h;
     scan_text_markup(u"<co=10>",h);check(h.numbers.decimal==10&&h.numbers.color_bits==16&&h.numeric.calls.size()==2,"scanner composes existing independent numeric parses");}
}
int main() {
    scanner_contracts();
    Host h;
    auto n = parse_text_tag_numbers(0,u"4a86e8",h);
    check(n.decimal == 0 && text_tag_argb(n.color_bits) == 0xff4a86e8U &&
          h.calls == std::vector<std::u16string>({u"D:4a86e8",u"H:4a86e8"}),
          "decimal failure resumes hex parse");
    n = parse_text_tag_numbers(0,u"10",h);
    check(n.decimal == 10 && n.color_bits == 16, "decimal and hex results are distinct");
    h.calls.clear();
    n = parse_text_tag_numbers(10,u" 1 , 2 , 3 ,ignored",h);
    check(n.color_bits == 0x010203 && h.calls == std::vector<std::u16string>(
          {u"D: 1 , 2 , 3 ,ignored",u"H: 1 , 2 , 3 ,ignored",u"D:1",u"D:2",u"D:3"}),
          "fallback trims channels, preserves order and ignores fourth field");
    n = parse_text_tag_numbers(0,u"1,256,0",h);
    check(text_tag_argb(n.color_bits) == 0xff010000U, "combine before extracting bytes, no channel clamp");
    n = parse_text_tag_numbers(0,u"1,2,-1",h);
    check(text_tag_argb(n.color_bits) == 0xffffffffU, "negative blue OR affects all low24 bits");
    n = parse_text_tag_numbers(0,u"1,2",h);
    check(n.color_bits == 0x010200, "two fields leave low byte zero");
    h.calls.clear();
    n = parse_text_tag_numbers(8,u"1,2,3",h);
    check(n.color_bits == 0 && h.calls.size() == 2, "non-color tags do not use RGB fallback");
    n = parse_text_tag_numbers(0,u"invalid",h);
    check(n.decimal == 0 && n.color_bits == 0, "two local failures with no comma remain zero");
    h.calls.clear();
    bool propagated = false;
    try { parse_text_tag_numbers(0,u"1,,3",h); }
    catch (const ManagedTextParseError&) { propagated = true; }
    check(propagated && h.calls.back() == u"D:" && h.calls.size() == 4,
          "component error propagates and third channel is not parsed");
    h.native_failure = true; h.calls.clear(); propagated = false;
    try { parse_text_tag_numbers(0,u"10",h); }
    catch (const std::logic_error&) { propagated = true; }
    check(propagated && h.calls.size() == 1, "unrelated native exceptions are not swallowed");
    std::cout << "text numeric parameter contracts passed\n";
}
