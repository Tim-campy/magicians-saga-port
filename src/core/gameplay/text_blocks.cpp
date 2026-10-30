#include "text_blocks.hpp"
#include "text_reveal.hpp"
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

namespace saga::core {
namespace {
static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559,"Text layout needs binary32");
float add(float a,float b){volatile float r=a+b;return r;}
float sub(float a,float b){volatile float r=a-b;return r;}
float mul(float a,float b){volatile float r=a*b;return r;}
std::int32_t wrap_add(std::int32_t a,std::int32_t b){
    const auto bits=static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b);
    std::int32_t result;std::memcpy(&result,&bits,sizeof result);return result;
}
std::int32_t arm_height(float value){
    constexpr auto low=std::numeric_limits<std::int32_t>::min();
    if(std::isnan(value))return 0;
    if(value==std::numeric_limits<float>::infinity())return low;
    if(value>=2147483648.0F)return std::numeric_limits<std::int32_t>::max();
    if(value<=-2147483648.0F)return low;
    return static_cast<std::int32_t>(value);
}
bool ascii_trim_space(char16_t c){return c==u' '||(c>=u'\t'&&c<=u'\r');}
class Builder final:public TextMarkupHost {
public:
    Builder(TextBlockLayout& output,const std::u16string& original,const TextBlockConfig& config,TextBlockHost& host)
        :output_(output),original_(original),config_(config),host_(host),color_(config.color),
         word_wrap_(!config.japanese_style&&original.find(u' ')!=std::u16string::npos){}
    bool equal_tag(std::u16string_view expected,std::u16string_view candidate)override{return host_.equal_tag(expected,candidate);}
    bool starts_with_tag(std::u16string_view candidate,std::u16string_view prefix)override{return host_.starts_with_tag(candidate,prefix);}
    void text(const TextMarkupSpan& span)override{
        for(auto i=span.source_begin;i<span.source_end;++i)unit(static_cast<std::size_t>(i));
    }
    void tag(const TextMarkupSpan& span)override{
        TextTagNumbers numbers{};
        if(span.has_parameter)numbers=parse_text_tag_numbers(span.tag_index,std::u16string(span.parameter),host_);
        if(span.tag_index==0)color_=text_tag_argb(numbers.color_bits);
        else if(span.tag_index==1)color_=config_.color;
        else throw std::logic_error("talk text layout tag path is unbound: "+std::to_string(span.tag_index));
        new_block_=true;
    }
    void run(){
        // 0x1bbd798 -> 0x1bbf394: empty ORIGINAL input returns before
        // translation or width reset, but replaces the block array.
        if(original_.empty()){output_.processed_source.clear();output_.blocks.clear();return;}
        output_.text_width=0;
        source_=host_.layout_text(original_);
        for(std::size_t at=0;(at=source_.find(u"<br>",at))!=std::u16string::npos;++at)source_.replace(at,4,u"\n");
        if(source_.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))throw std::length_error("text layout source exceeds managed string length");
        for(const auto c:source_)if(c>127)throw std::logic_error("talk text layout non-ASCII pair path is unbound");
        scan_text_markup(source_,*this);
        if(line_first_<blocks_.size())finish_line(false);
        const auto vertical=config_.alignment&0x70U;
        float shift=0;
        if(vertical==0x20U)shift=config_.height==-1?mul(line_y_,-0.5F):mul(sub(config_.height,line_y_),0.5F);
        else if(vertical==0x40U)shift=config_.height==-1?-line_y_:sub(config_.height,line_y_);
        if(config_.height!=-1&&shift<0)shift=0;
        output_.processed_source=source_;
        output_.blocks=std::move(blocks_); // Native +0x10 is assigned before the final per-block substring pass.
        for(auto& block:output_.blocks){
            block.y=add(shift,block.y);
            if(block.source_begin!=-1){
                const auto begin=static_cast<std::size_t>(block.source_begin),length=static_cast<std::size_t>(block.source_length);
                if(block.source_begin<0||block.source_length<0||begin>source_.size()||length>source_.size()-begin)throw std::out_of_range("text block source span");
                block.text=source_.substr(begin,length);
            }
        }
    }
private:
    bool starts_at(std::size_t at,std::u16string_view prefix){
        return host_.starts_with_tag(std::u16string_view(source_).substr(at),prefix);
    }
    bool prior_visible_source(std::size_t before){
        // 0x1bbe224..0x1bbe310 scans ORIGINAL text, but skips a tag using the
        // transformed source's closing delimiter. Preserve that source mix.
        for(std::size_t at=0;at<before;++at){
            const auto c=original_.at(at);
            if(ascii_trim_space(c))continue;
            if(c!=u'<')return true;
            at=source_.find(u'>',at+1);
            if(at==std::u16string::npos)return false;
        }
        return false;
    }
    float upcoming_word(std::size_t at,std::size_t end){
        static constexpr std::array<std::u16string_view,9> ignored{
            u"<co=",u"<si=",u"<st=",u"<po=",u"<bl=",u"<bo=",u"<ff>",u"</",u"<br>"};
        float width=0;
        for(auto i=at;i<end;++i){
            const auto c=source_[i];
            if(c==u'<'){
                const auto closing=source_.find(u'>',i+1);
                if(closing!=std::u16string::npos)i=closing;
                // The native cursor is already at '>' for these tests. Keep
                // that quirk, including measurement of the captured '<'.
                bool skip=false;for(const auto tag:ignored)if(starts_at(i,tag)){skip=true;break;}
                if(skip)continue;
                if(starts_at(i,u"<pic=")||starts_at(i,u"<btn="))width=add(width,0);
            }
            width=add(width,host_.measure_run(std::u16string(1,c),config_.font_size));
        }
        return width;
    }
    void finish_line(bool advance){
        float x=0;
        const auto horizontal=config_.alignment&7U;
        if(horizontal==2)x=config_.width==-1?add(mul(line_width_,-0.5F),0):sub(mul(config_.width,0.5F),mul(line_width_,0.5F));
        else if(horizontal==4)x=sub(config_.width==-1?0:config_.width,line_width_);
        for(auto i=line_first_;i<blocks_.size();++i){auto& b=blocks_[i];b.x=x;b.y=line_y_;x=add(x,b.width);}
        line_first_=blocks_.size();
        line_y_=add(line_y_,static_cast<float>(advance?wrap_add(config_.font_size,config_.line_space):config_.font_size));
        if(advance){line_width_=0;new_block_=true;}
    }
    float increment(char16_t c){
        std::u16string candidate;
        float previous=0;
        if(current_!=std::numeric_limits<std::size_t>::max()){candidate=blocks_[current_].text;previous=blocks_[current_].width;}
        candidate.push_back(c);return sub(host_.measure_run(candidate,config_.font_size),previous);
    }
    void unit(std::size_t at){
        const auto c=source_[at];
        float delta=(c==u'\n'||c==u'\r')?0:increment(c);
        bool break_line=c==u'\n'||(config_.width!=-1&&add(line_width_,delta)>config_.width);
        if(c!=u' '&&word_boundary_&&word_wrap_){
            auto end=source_.size();
            for(auto i=at;i<source_.size();++i)if(source_[i]==u' '||source_[i]==u'\n'||starts_at(i,u"<br>")){end=i;break;}
            if(end>at){const auto word=upcoming_word(at,end);
                if(config_.width!=-1&&add(line_width_,word)>config_.width){
                    if(prior_visible_source(at))break_line=true;
                    else word_wrap_=false; // First overlong word disables later word lookahead.
                }
            }
            word_boundary_=false;
        }
        const bool english_space=!config_.japanese_style&&c==u' ';
        if(break_line&&english_space)delta=0;
        if(break_line)finish_line(true);
        if(delta>0){
            if(new_block_){
                LayoutTextBlock block;block.font_size=config_.font_size;block.color=color_;blocks_.push_back(std::move(block));
                current_=blocks_.size()-1;new_block_=false;
            }
            auto& block=blocks_.at(current_);
            delta=increment(c); // Native emits another cumulative-run query after allocating/selecting the block.
            if(block.source_begin==-1)block.source_begin=static_cast<std::int32_t>(at);
            block.source_length=static_cast<std::int32_t>(at)-block.source_begin+1;
            block.text.push_back(c);block.width=add(block.width,delta);line_width_=add(line_width_,delta);
            if(output_.text_width<line_width_)output_.text_width=line_width_;
        }
        word_boundary_=word_boundary_||english_space;
    }
    TextBlockLayout& output_;const std::u16string& original_;const TextBlockConfig& config_;TextBlockHost& host_;
    std::u16string source_;std::vector<LayoutTextBlock> blocks_;
    std::size_t line_first_=0,current_=std::numeric_limits<std::size_t>::max();
    float line_width_=0,line_y_=0;std::uint32_t color_;
    bool word_wrap_,word_boundary_=true,new_block_=true;
};
}
TextBlockLayout& build_talk_text_blocks(TextBlockLayout& out,const std::u16string& original,const TextBlockConfig& config,TextBlockHost& host){
    Builder(out,original,config,host).run();return out;
}
std::int32_t text_blocks_height(const TextBlockLayout& layout){
    // TextLayout.GetHeight 0x1bbfd2c: empty text does not consume the font.
    float maximum=0;
    for(const auto& block:layout.blocks)if(!block.text.empty()){
        const auto candidate=add(block.y,static_cast<float>(block.font_size));if(candidate>maximum)maximum=candidate;
    }
    return arm_height(add(maximum,0.9F));
}
std::int32_t text_blocks_reveal_length(const TextBlockLayout& layout){
    std::vector<RevealBlock> values;values.reserve(layout.blocks.size());
    for(const auto& block:layout.blocks)values.push_back({false,false,&block.text});
    std::vector<const RevealBlock*> pointers;pointers.reserve(values.size());for(const auto& value:values)pointers.push_back(&value);
    return reveal_length(pointers);
}
}
