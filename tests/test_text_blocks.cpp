#include "core/gameplay/text_blocks.hpp"
#include "core/gameplay/event_catalog.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace saga::core;
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::abort();}}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught,"expected text block failure");}
struct Host final:TextBlockHost {
    std::vector<std::u16string> runs;
    bool kern=false,fail_measure=false,fail_layout=false;
    int layout_calls=0;
    TextBlockLayout* observed_layout=nullptr;
    float width_at_layout=-1;
    std::u16string layout_text(const std::u16string& text)override{
        ++layout_calls;if(observed_layout)width_at_layout=observed_layout->text_width;
        if(fail_layout)throw std::runtime_error("layout");return text;
    }
    bool equal_tag(std::u16string_view a,std::u16string_view b)override{return a==b;}
    bool starts_with_tag(std::u16string_view candidate,std::u16string_view prefix)override{return candidate.substr(0,prefix.size())==prefix;}
    std::int32_t parse_decimal(const std::u16string& text)override{
        std::string input(text.begin(),text.end());try{return parse_event_decimal(input);}catch(const std::exception&){throw ManagedTextParseError("decimal");}
    }
    std::int32_t parse_hexadecimal(const std::u16string& text)override{
        try{std::size_t end=0;const auto n=std::stoul(std::string(text.begin(),text.end()),&end,16);if(end!=text.size())throw std::invalid_argument("suffix");return static_cast<std::int32_t>(n);}
        catch(const std::exception&){throw ManagedTextParseError("hex");}
    }
    float measure_run(const std::u16string& text,std::int32_t size)override{
        check(size==12,"configured numeric font size");runs.push_back(text);
        if(fail_measure)throw std::runtime_error("measure");
        float width=static_cast<float>(text.size());
        if(kern&&text.find(u"AV")!=std::u16string::npos)width-=0.25F;
        return width;
    }
};
TextBlockConfig config(float width=196){return {width,60,4,12,0xff252336U,0x11U,false};}
int main(){
    {Host h;TextBlockLayout out;LayoutTextBlock old;old.text=u"old";out.blocks.push_back(old);
     out.processed_source=u"old";out.text_width=99;h.fail_layout=true;h.fail_measure=true;
     build_talk_text_blocks(out,u"",config(),h);
     check(out.blocks.empty()&&out.processed_source.empty()&&out.text_width==99,"empty original clears blocks and retains prior width");
     check(h.layout_calls==0&&h.runs.empty(),"empty original bypasses translation and metrics");
     out.blocks.push_back(old);h.observed_layout=&out;
     fails([&]{build_talk_text_blocks(out,u"a",config(),h);});
     check(h.layout_calls==1&&h.width_at_layout==0&&out.blocks[0].text==u"old","nonempty resets width before translation callback failure");}
    {Host h;TextBlockLayout out;build_talk_text_blocks(out,u"a\rb",config(),h);
     check(out.blocks.size()==1&&out.blocks[0].text==u"a\rb"&&out.blocks[0].width==2,"CR skips emission but final source span retains internal CR");
     check(h.runs==std::vector<std::u16string>{u"a",u"a",u"ab",u"ab"}&&text_blocks_reveal_length(out)==3,"CR does not measure or break a normal line");}
    {Host h;TextBlockLayout out;build_talk_text_blocks(out,u"AV",config(),h);
     check(out.blocks.size()==1&&out.blocks[0].text==u"AV"&&out.blocks[0].width==2,"plain run");
     check(h.runs==std::vector<std::u16string>{u"A",u"A",u"AV",u"AV"},"two ordered cumulative queries per ordinary unit");
     check(text_blocks_height(out)==12&&text_blocks_reveal_length(out)==2,"numeric height/reveal kernels");}
    {Host h;h.kern=true;TextBlockLayout out;build_talk_text_blocks(out,u"AV",config(1.8F),h);
     check(out.blocks.size()==1&&out.blocks[0].text==u"AV"&&out.blocks[0].width==1.75F,"cumulative kerning affects wrap");}
    {Host h;TextBlockLayout out;build_talk_text_blocks(out,u"aa bb",config(4),h);
     check(out.blocks.size()==2&&out.blocks[0].text==u"aa "&&out.blocks[1].text==u"bb","word lookahead retains preceding space");
     check(out.blocks[0].y==0&&out.blocks[1].y==16&&text_blocks_height(out)==28,"line geometry and native numeric font height");
     check(text_blocks_reveal_length(out)==5,"retained space contributes to reveal");}
    {Host h;TextBlockLayout out;build_talk_text_blocks(out,u"a b",config(1),h);
     check(out.blocks.size()==2&&out.blocks[0].text==u"a"&&out.blocks[1].text==u"b","overflowing English space drops");
     check(text_blocks_reveal_length(out)==2,"dropped wrapping space not counted");}
    {Host h;TextBlockLayout out;build_talk_text_blocks(out,u"aaaaa bb",config(3),h);
     check(out.blocks.size()==3&&out.blocks[0].text==u"aaa"&&out.blocks[1].text==u"aa "&&out.blocks[2].text==u"bb","first overlong word disables later lookahead");}
    {Host h;TextBlockLayout out;build_talk_text_blocks(out,u"a<br>b",config(),h);
     check(out.processed_source==u"a\nb"&&out.blocks.size()==2,"literal br preprocessing");
     check(out.blocks[0].text==u"a"&&out.blocks[1].text==u"b"&&out.blocks[1].y==16,"explicit line break");}
    {Host h;TextBlockLayout out;build_talk_text_blocks(out,u"a<co=00ff00>b</co>c",config(),h);
     check(out.blocks.size()==3&&out.blocks[0].text==u"a"&&out.blocks[1].text==u"b"&&out.blocks[2].text==u"c","color tags split source runs");
     check(out.blocks[0].color==0xff252336U&&out.blocks[1].color==0xff00ff00U&&out.blocks[2].color==0xff252336U,"closing color restores original");
     check(out.blocks[1].x==1&&out.blocks[2].x==2&&text_blocks_reveal_length(out)==3,"colored geometry excludes markup");}
    {Host h;TextBlockLayout out;build_talk_text_blocks(out,u"a bc<co=00ff00>d",config(5.5F),h);
     check(out.blocks.size()==3&&out.blocks[0].text==u"a "&&out.blocks[1].text==u"bc"&&out.blocks[1].y==16,"lookahead retains captured opener width after closing-index tag test");}
    {Host h;TextBlockLayout out;const auto source=u"a"+std::u16string(u"<")+std::u16string(33,u'x')+u">";
     build_talk_text_blocks(out,source,config(),h);
     check(out.blocks.size()==1&&out.blocks[0].text==source,"final source substring preserves overlong opener inside span");
     check(out.blocks[0].width==static_cast<float>(source.size()-1),"overlong opener excluded from width emission");}
    {Host h;TextBlockLayout out;build_talk_text_blocks(out,u"<unknown>a",config(),h);
     check(out.blocks.size()==1&&out.blocks[0].text==u"<unknown>a","unknown short tag remains text");}
    {Host h;TextBlockLayout out;auto c=config(10);c.alignment=0x22;
     build_talk_text_blocks(out,u"ab",c,h);check(out.blocks[0].x==4&&out.blocks[0].y==24,"center/middle alignment");}
    {Host h;TextBlockLayout out;LayoutTextBlock old;old.text=u"old";out.blocks.push_back(old);out.text_width=99;
     fails([&]{build_talk_text_blocks(out,u"abc<co=1,bad,3>x",config(),h);});
     check(out.text_width==3&&out.blocks.size()==1&&out.blocks[0].text==u"old","pre-array failure retains old blocks but new width");
     fails([&]{build_talk_text_blocks(out,u"<si=2>x",config(),h);});
     fails([&]{build_talk_text_blocks(out,u"\u00e9",config(),h);});}
    {TextBlockLayout out;LayoutTextBlock a;a.text=u"x";a.font_size=12;a.y=0.2F;out.blocks.push_back(a);
     check(text_blocks_height(out)==13,"height adds binary32 point9 before conversion");
     out.blocks[0].text.clear();check(text_blocks_height(out)==0,"empty block skipped by height");}
    {TextBlockLayout out;LayoutTextBlock a;a.text=u"x";a.font_size=12;a.y=-100;out.blocks.push_back(a);
     check(text_blocks_height(out)==0,"negative block bottom does not lower zero maximum");
     out.blocks[0].y=std::numeric_limits<float>::quiet_NaN();check(text_blocks_height(out)==0,"NaN block bottom does not replace maximum");
     out.blocks[0].y=std::numeric_limits<float>::infinity();check(text_blocks_height(out)==std::numeric_limits<std::int32_t>::min(),"native height positive infinity conversion quirk");}
    std::cout<<"text blocks contract ok\n";
}
