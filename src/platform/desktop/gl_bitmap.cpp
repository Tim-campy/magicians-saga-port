#include "gl_bitmap.hpp"
#include "bitmap_mesh_frame.hpp"
#define GL_GLEXT_PROTOTYPES 1
#include <SDL3/SDL_opengl.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <stdexcept>

namespace saga::desktop {
void bind_bitmap_mesh_frame(GlMeshBindings& bindings,const BitmapMeshFrame& frame){
    bindings.materials.clear();bindings.groups.clear();
    for(const auto& entry:frame.materials())bindings.materials.emplace(entry.first,entry.second);
    for(const auto& entry:frame.uploaded())bindings.groups.emplace(entry.first,GlMeshQuads{entry.second.data(),entry.second.size()});
}
namespace {
void require(bool value,const char* name){if(!value)throw std::runtime_error(std::string(name)+": "+SDL_GetError());}
// SDL's public loader owns platform lookup. Function pointers stay with their context.
#define SAGA_GL_FUNCTIONS(X) \
 X(GetError) X(GetString) X(GetIntegerv) X(Enable) X(Disable) X(Viewport) X(Scissor) X(Clear) X(ClearColor) X(ClearDepth) \
 X(DepthMask) X(DepthFunc) X(DepthRange) X(BlendFuncSeparate) X(BlendEquationSeparate) X(ColorMask) \
 X(GenTextures) X(DeleteTextures) X(BindTexture) X(TexImage2D) X(TexParameteri) X(PixelStorei) X(ActiveTexture) \
 X(GenFramebuffers) X(DeleteFramebuffers) X(BindFramebuffer) X(FramebufferTexture2D) X(CheckFramebufferStatus) X(BlitFramebuffer) \
 X(GenRenderbuffers) X(DeleteRenderbuffers) X(BindRenderbuffer) X(RenderbufferStorage) X(FramebufferRenderbuffer) \
 X(CreateShader) X(ShaderSource) X(CompileShader) X(GetShaderiv) X(GetShaderInfoLog) X(DeleteShader) \
 X(CreateProgram) X(AttachShader) X(LinkProgram) X(GetProgramiv) X(GetProgramInfoLog) X(DeleteProgram) X(UseProgram) \
 X(GetUniformLocation) X(Uniform1i) X(UniformMatrix4fv) X(GenVertexArrays) X(DeleteVertexArrays) X(BindVertexArray) \
 X(GenBuffers) X(DeleteBuffers) X(BindBuffer) X(BufferData) X(BufferSubData) X(EnableVertexAttribArray) X(VertexAttribPointer) X(DrawArrays) X(ReadPixels)
struct Gl {
#define DECLARE(name) decltype(&::gl##name) name=nullptr;
    SAGA_GL_FUNCTIONS(DECLARE)
#undef DECLARE
    Gl(){
#define LOAD(name) name=reinterpret_cast<decltype(name)>(SDL_GL_GetProcAddress("gl" #name));if(!name)throw std::runtime_error("missing GL3.3 function gl" #name);
        SAGA_GL_FUNCTIONS(LOAD)
#undef LOAD
    }
    void check(const char* stage){const auto e=GetError();if(e!=GL_NO_ERROR)throw std::runtime_error(std::string(stage)+": GL error "+std::to_string(e));}
};
#undef SAGA_GL_FUNCTIONS
constexpr const char* vertex_source=R"GLSL(#version 330 core
layout(location=0) in vec3 point;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec4 tint;
uniform mat4 view_projection;
uniform mat4 model;
out vec2 uv;
out vec4 color;
void main(){gl_Position=view_projection*model*vec4(point,1.0);uv=texcoord;color=tint;}
)GLSL";
constexpr const char* fragment_source=R"GLSL(#version 330 core
in vec2 uv;
in vec4 color;
uniform sampler2D image;
uniform int modulate;
uniform int alpha_test;
out vec4 pixel;
void main(){
    vec4 value=texture(image,uv);
    if(modulate!=0)value*=color;
    if(alpha_test==1&&!(value.a>=0.01))discard;
    if(alpha_test==2&&!(value.a>0.99))discard;
    if(alpha_test==3&&!(value.a<=0.99))discard;
    pixel=value;
}
)GLSL";
void finite_matrix(const core::OffscreenMatrix& m){for(float v:m.values)if(!std::isfinite(v))throw std::invalid_argument("nonfinite GL matrix");}
GLuint compile(Gl& gl,GLenum type,const char* source){
    const auto shader=gl.CreateShader(type);if(!shader)throw std::runtime_error("GL shader allocation failed");
    gl.ShaderSource(shader,1,&source,nullptr);gl.CompileShader(shader);GLint ok=0;gl.GetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if(!ok){std::array<char,8192> log{};gl.GetShaderInfoLog(shader,static_cast<GLsizei>(log.size()),nullptr,log.data());gl.DeleteShader(shader);throw std::runtime_error(std::string("GL shader compile: ")+log.data());}
    return shader;
}
struct Vertex {float x,y,z,u,v,r,g,b,a;};
}
void configure_gl_bitmap_context(){
    require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3),"GL major");require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3),"GL minor");
    require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE),"GL core profile");
    require(SDL_GL_SetAttribute(SDL_GL_RED_SIZE,8)&&SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,8)&&SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,8)&&SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,8),"GL RGBA");
    require(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,0)&&SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE,0),"GL target profile");
}
struct GlBitmapRenderer::Impl {
    SDL_Window& window;std::int32_t width,height;Textures textures;SDL_GLContext context=nullptr;std::unique_ptr<Gl> gl;
    GLuint target=0,color=0,depth=0,program=0,vao=0,vbo=0;GLint vp_uniform=-1,model_uniform=-1,modulate_uniform=-1,test_uniform=-1;
    std::map<std::size_t,GLuint> uploaded;std::uint32_t active_key=0;bool begun=false,material_ready=false;
    Impl(SDL_Window& w,std::int32_t x,std::int32_t y,Textures t):window(w),width(x),height(y),textures(std::move(t)){}
    ~Impl(){
        if(context){if(SDL_GL_MakeCurrent(&window,context)&&gl){
            for(const auto& entry:uploaded)gl->DeleteTextures(1,&entry.second);
            if(vbo)gl->DeleteBuffers(1,&vbo);if(vao)gl->DeleteVertexArrays(1,&vao);if(program)gl->DeleteProgram(program);
            if(depth)gl->DeleteRenderbuffers(1,&depth);if(color)gl->DeleteTextures(1,&color);if(target)gl->DeleteFramebuffers(1,&target);
        }SDL_GL_DestroyContext(context);}
    }
    void current(){if(SDL_GL_GetCurrentContext()!=context||SDL_GL_GetCurrentWindow()!=&window)require(SDL_GL_MakeCurrent(&window,context),"GL current context");}
    void ready(){if(!begun)throw std::logic_error("GL frame has not begun");current();}
    void init(){
        if(width<=0||height<=0||width>8192||height>8192||static_cast<std::int64_t>(width)*height>16*1024*1024||!textures)throw std::invalid_argument("invalid GL target or texture provider");
        context=SDL_GL_CreateContext(&window);require(context!=nullptr,"SDL_GL_CreateContext");gl=std::make_unique<Gl>();auto& g=*gl;
        GLint major=0,minor=0;g.GetIntegerv(GL_MAJOR_VERSION,&major);g.GetIntegerv(GL_MINOR_VERSION,&minor);
        if(major<3||(major==3&&minor<3))throw std::runtime_error("OpenGL 3.3 required");
        GLuint vs=0,fs=0;
        try{vs=compile(g,GL_VERTEX_SHADER,vertex_source);fs=compile(g,GL_FRAGMENT_SHADER,fragment_source);program=g.CreateProgram();
            if(!program)throw std::runtime_error("GL program allocation failed");g.AttachShader(program,vs);g.AttachShader(program,fs);g.LinkProgram(program);
            GLint ok=0;g.GetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok){std::array<char,8192> log{};g.GetProgramInfoLog(program,static_cast<GLsizei>(log.size()),nullptr,log.data());throw std::runtime_error(std::string("GL program link: ")+log.data());}
        }catch(...){if(vs)g.DeleteShader(vs);if(fs)g.DeleteShader(fs);throw;}g.DeleteShader(vs);g.DeleteShader(fs);
        g.UseProgram(program);vp_uniform=g.GetUniformLocation(program,"view_projection");model_uniform=g.GetUniformLocation(program,"model");
        modulate_uniform=g.GetUniformLocation(program,"modulate");test_uniform=g.GetUniformLocation(program,"alpha_test");
        const auto image_uniform=g.GetUniformLocation(program,"image");if(std::min({vp_uniform,model_uniform,modulate_uniform,test_uniform,image_uniform})<0)throw std::runtime_error("GL required uniform missing");g.Uniform1i(image_uniform,0);
        g.GenVertexArrays(1,&vao);g.BindVertexArray(vao);g.GenBuffers(1,&vbo);g.BindBuffer(GL_ARRAY_BUFFER,vbo);g.BufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(6*sizeof(Vertex)),nullptr,GL_STREAM_DRAW);
        g.EnableVertexAttribArray(0);g.VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,x)));
        g.EnableVertexAttribArray(1);g.VertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,u)));
        g.EnableVertexAttribArray(2);g.VertexAttribPointer(2,4,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,r)));
        g.GenFramebuffers(1,&target);g.BindFramebuffer(GL_FRAMEBUFFER,target);
        g.GenTextures(1,&color);g.BindTexture(GL_TEXTURE_2D,color);g.TexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        g.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);g.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        g.FramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
        g.GenRenderbuffers(1,&depth);g.BindRenderbuffer(GL_RENDERBUFFER,depth);g.RenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,width,height);g.FramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
        if(g.CheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("GL RGBA8/D24 target incomplete");g.check("GL renderer initialization");
    }
    GLuint texture(std::size_t key){
        const auto found=uploaded.find(key);if(found!=uploaded.end())return found->second;
        const auto binding=textures(key);if(!binding.pixels)throw std::invalid_argument("missing GL source pixels");const auto& p=*binding.pixels;
        if((binding.wrap_x!=GlBitmapWrap::clamp&&binding.wrap_x!=GlBitmapWrap::repeat)||(binding.wrap_y!=GlBitmapWrap::clamp&&binding.wrap_y!=GlBitmapWrap::repeat))throw std::invalid_argument("unsupported GL texture wrapping");
        GLint limit=0;gl->GetIntegerv(GL_MAX_TEXTURE_SIZE,&limit);
        if(p.width<=0||p.height<=0||p.width>limit||p.height>limit||static_cast<std::uint64_t>(p.width)*static_cast<std::uint64_t>(p.height)*4U!=p.rgba.size())throw std::invalid_argument("invalid GL source pixels");
        GLuint id=0;gl->GenTextures(1,&id);gl->BindTexture(GL_TEXTURE_2D,id);gl->PixelStorei(GL_UNPACK_ALIGNMENT,1);
        gl->TexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,p.width,p.height,0,GL_RGBA,GL_UNSIGNED_BYTE,p.rgba.data());
        gl->TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);gl->TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        gl->TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,binding.wrap_x==GlBitmapWrap::repeat?GL_REPEAT:GL_CLAMP_TO_EDGE);
        gl->TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,binding.wrap_y==GlBitmapWrap::repeat?GL_REPEAT:GL_CLAMP_TO_EDGE);
        try{gl->check("GL texture upload");uploaded.emplace(key,id);}catch(...){gl->DeleteTextures(1,&id);throw;}return id;
    }
};
GlBitmapRenderer::GlBitmapRenderer(SDL_Window& w,std::int32_t width,std::int32_t height,Textures textures):impl_(std::make_unique<Impl>(w,width,height,std::move(textures))){impl_->init();}
GlBitmapRenderer::~GlBitmapRenderer()=default;
void GlBitmapRenderer::resize_target(std::int32_t width,std::int32_t height){
    if(width<1||height<1||width>8192||height>8192||static_cast<std::int64_t>(width)*height>16*1024*1024)throw std::invalid_argument("invalid GL resize target");
    auto& s=*impl_;if(width==s.width&&height==s.height)return;s.current();auto& g=*s.gl;
    g.BindFramebuffer(GL_FRAMEBUFFER,s.target);g.BindTexture(GL_TEXTURE_2D,s.color);
    g.TexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    g.BindRenderbuffer(GL_RENDERBUFFER,s.depth);g.RenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,width,height);
    if(g.CheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("GL resized target incomplete");
    g.check("GL target resize");s.width=width;s.height=height;s.begun=false;s.material_ready=false;
}
void GlBitmapRenderer::begin_frame(GlBitmapClear clear,core::RenderColor background,const core::OffscreenMatrix& vp){
    if(clear!=GlBitmapClear::color_and_depth&&clear!=GlBitmapClear::depth&&clear!=GlBitmapClear::preserve)throw std::invalid_argument("unsupported GL frame clear");
    finite_matrix(vp);auto& s=*impl_;s.current();auto& g=*s.gl;g.BindFramebuffer(GL_FRAMEBUFFER,s.target);g.UseProgram(s.program);g.BindVertexArray(s.vao);
    g.Disable(GL_CULL_FACE);g.Disable(GL_DITHER);g.Disable(GL_MULTISAMPLE);g.Disable(GL_FRAMEBUFFER_SRGB);g.Enable(GL_DEPTH_TEST);g.DepthFunc(GL_LEQUAL);g.DepthRange(0,1);
    g.Enable(GL_SCISSOR_TEST);g.Viewport(0,0,s.width,s.height);g.Scissor(0,0,s.width,s.height);g.ColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);g.DepthMask(GL_TRUE);
    if(clear!=GlBitmapClear::preserve){g.ClearDepth(1);if(clear==GlBitmapClear::color_and_depth){g.ClearColor(background.r,background.g,background.b,background.a);g.Clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);}else g.Clear(GL_DEPTH_BUFFER_BIT);}
    g.UniformMatrix4fv(s.vp_uniform,1,GL_FALSE,vp.values.data());
    s.begun=true;s.material_ready=false;g.check("GL begin frame");
}
void GlBitmapRenderer::set_viewport(std::int32_t x,std::int32_t y,std::int32_t width,std::int32_t height){
    auto& s=*impl_;if(x<0||y<0||width<=0||height<=0||static_cast<std::int64_t>(x)+width>s.width||static_cast<std::int64_t>(y)+height>s.height)throw std::invalid_argument("GL viewport outside target");s.ready();s.gl->Viewport(x,y,width,height);s.gl->Scissor(x,y,width,height);
}
void GlBitmapRenderer::clear_depth(){auto& s=*impl_;s.ready();s.gl->DepthMask(GL_TRUE);s.gl->ClearDepth(1);s.gl->Clear(GL_DEPTH_BUFFER_BIT);s.gl->check("GL clear depth");s.material_ready=false;}
void GlBitmapRenderer::set_material(std::uint32_t key){
    const auto p=core::sprite_pixel_pipeline(key);auto& s=*impl_;s.ready();auto& g=*s.gl;g.UseProgram(s.program);g.DepthMask(p.depth_write?GL_TRUE:GL_FALSE);
    using core::SpriteBlend;g.Enable(GL_BLEND);GLenum src=GL_SRC_ALPHA,dst=GL_ONE_MINUS_SRC_ALPHA,sa=GL_ONE,da=GL_ONE_MINUS_SRC_ALPHA,op=GL_FUNC_ADD;
    switch(p.blend){case SpriteBlend::replace:g.Disable(GL_BLEND);break;case SpriteBlend::alpha:break;
        case SpriteBlend::add:dst=GL_ONE;sa=GL_SRC_ALPHA;da=GL_ONE;break;
        case SpriteBlend::subtract_alpha:dst=GL_ONE;op=GL_FUNC_REVERSE_SUBTRACT;break;
        case SpriteBlend::subtract_add:dst=GL_ONE;sa=GL_SRC_ALPHA;da=GL_ONE;op=GL_FUNC_REVERSE_SUBTRACT;break;
        case SpriteBlend::attenuate:src=GL_ZERO;break;}
    g.BlendFuncSeparate(src,dst,sa,da);g.BlendEquationSeparate(op,op);g.Uniform1i(s.modulate_uniform,p.modulate?1:0);g.Uniform1i(s.test_uniform,static_cast<int>(p.alpha_test));
    s.active_key=key;s.material_ready=true;g.check("GL material state");
}
void GlBitmapRenderer::draw(const BitmapQuad& quad,const core::OffscreenMatrix& model){
    auto& s=*impl_;s.ready();if(!quad.native_depth||!s.material_ready||quad.shader_key!=s.active_key)throw std::invalid_argument("GL draw requires native depth and matching material");
    const auto p=core::sprite_pixel_pipeline(quad.shader_key);if(p.blend!=quad.pipeline.blend||p.alpha_test!=quad.pipeline.alpha_test||p.modulate!=quad.pipeline.modulate||p.depth_write!=quad.pipeline.depth_write)throw std::invalid_argument("inconsistent GL bitmap pipeline");finite_matrix(model);
    const auto& c=quad.color;const auto& uv=quad.uv;const auto coords=quad.native_uv.value_or(std::array<std::array<float,2>,4>{{{uv.left,1-uv.bottom},{uv.right,1-uv.bottom},{uv.right,1-uv.top},{uv.left,1-uv.top}}});
    constexpr std::array<std::size_t,6> indices{0,1,2,0,2,3};std::array<Vertex,6> vertices;
    for(std::size_t i=0;i<indices.size();++i){const auto j=indices[i];const auto& v=quad.points[j];vertices[i]={v.x,v.y,v.z,coords[j][0],coords[j][1],c.r,c.g,c.b,c.a};
        for(float f:{v.x,v.y,v.z,coords[j][0],coords[j][1],c.r,c.g,c.b,c.a})if(!std::isfinite(f))throw std::invalid_argument("nonfinite GL bitmap vertex");}
    auto& g=*s.gl;g.ActiveTexture(GL_TEXTURE0);const auto texture=s.texture(quad.texture);g.BindTexture(GL_TEXTURE_2D,texture);g.BindVertexArray(s.vao);g.BindBuffer(GL_ARRAY_BUFFER,s.vbo);g.BufferSubData(GL_ARRAY_BUFFER,0,sizeof(vertices),vertices.data());
    g.UniformMatrix4fv(s.model_uniform,1,GL_FALSE,model.values.data());g.DrawArrays(GL_TRIANGLES,0,6);g.check("GL bitmap draw");
}
DecodedImage GlBitmapRenderer::read_pixels(){
    auto& s=*impl_;s.ready();DecodedImage out{s.width,s.height,{}};const auto stride=static_cast<std::size_t>(s.width)*4;out.rgba.resize(stride*static_cast<std::size_t>(s.height));
    s.gl->PixelStorei(GL_PACK_ALIGNMENT,1);s.gl->ReadPixels(0,0,s.width,s.height,GL_RGBA,GL_UNSIGNED_BYTE,out.rgba.data());s.gl->check("GL bitmap readback");
    for(std::int32_t y=0;y<s.height/2;++y){auto* a=out.rgba.data()+static_cast<std::size_t>(y)*stride;auto* b=out.rgba.data()+static_cast<std::size_t>(s.height-1-y)*stride;std::swap_ranges(a,a+stride,b);}return out;
}
void GlBitmapRenderer::present(){auto& s=*impl_;s.ready();int width=0,height=0;require(SDL_GetWindowSizeInPixels(&s.window,&width,&height),"GL window size");auto& g=*s.gl;
    g.Disable(GL_SCISSOR_TEST);g.BindFramebuffer(GL_READ_FRAMEBUFFER,s.target);g.BindFramebuffer(GL_DRAW_FRAMEBUFFER,0);g.BlitFramebuffer(0,0,s.width,s.height,0,0,width,height,GL_COLOR_BUFFER_BIT,GL_NEAREST);
    require(SDL_GL_SwapWindow(&s.window),"GL swap window");g.BindFramebuffer(GL_FRAMEBUFFER,s.target);g.Enable(GL_SCISSOR_TEST);g.check("GL present");}
DecodedImage GlBitmapRenderer::read_pixels(std::int32_t width,std::int32_t height){
    auto& s=*impl_;if(width==s.width&&height==s.height)return read_pixels();
    if(width<1||height<1||width>8192||height>8192||static_cast<std::int64_t>(width)*height>16*1024*1024)throw std::invalid_argument("invalid presented capture size");
    s.ready();auto& g=*s.gl;GLuint target=0,color=0;
    const auto restore=[&]{g.BindFramebuffer(GL_FRAMEBUFFER,s.target);g.Enable(GL_SCISSOR_TEST);if(color)g.DeleteTextures(1,&color);if(target)g.DeleteFramebuffers(1,&target);};
    try{
        g.GenFramebuffers(1,&target);g.BindFramebuffer(GL_FRAMEBUFFER,target);
        g.GenTextures(1,&color);g.BindTexture(GL_TEXTURE_2D,color);g.TexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        g.FramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
        if(g.CheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("presented capture target incomplete");
        g.Disable(GL_SCISSOR_TEST);g.BindFramebuffer(GL_READ_FRAMEBUFFER,s.target);g.BindFramebuffer(GL_DRAW_FRAMEBUFFER,target);
        g.BlitFramebuffer(0,0,s.width,s.height,0,0,width,height,GL_COLOR_BUFFER_BIT,GL_NEAREST);
        g.BindFramebuffer(GL_FRAMEBUFFER,target);DecodedImage out{width,height,{}};const auto stride=static_cast<std::size_t>(width)*4;
        out.rgba.resize(stride*static_cast<std::size_t>(height));g.PixelStorei(GL_PACK_ALIGNMENT,1);
        g.ReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,out.rgba.data());g.check("presented capture readback");
        for(std::int32_t y=0;y<height/2;++y){auto* a=out.rgba.data()+static_cast<std::size_t>(y)*stride;auto* b=out.rgba.data()+static_cast<std::size_t>(height-1-y)*stride;std::swap_ranges(a,a+stride,b);}
        restore();return out;
    }catch(...){restore();throw;}
}
std::string GlBitmapRenderer::driver()const{auto& s=*impl_;s.current();const auto* version=s.gl->GetString(GL_VERSION);const auto* renderer=s.gl->GetString(GL_RENDERER);if(!version||!renderer)throw std::runtime_error("GL driver information unavailable");return std::string(reinterpret_cast<const char*>(renderer))+" / "+reinterpret_cast<const char*>(version);}
GlMeshSubmissionHost::GlMeshSubmissionHost(GlBitmapRenderer& renderer,const GlMeshBindings& bindings,Diagnostics diagnostics,std::function<std::int32_t()> counter):renderer_(renderer),bindings_(bindings),diagnostics_(std::move(diagnostics)),check_count_(std::move(counter)){if(!diagnostics_||!check_count_)throw std::invalid_argument("missing GL submission service");}
void GlMeshSubmissionHost::clear_depth(){renderer_.clear_depth();}
void GlMeshSubmissionHost::set_material_pass(core::SubmissionMaterial& material){renderer_.set_material(bindings_.materials.at(&material));}
void GlMeshSubmissionHost::draw_mesh(core::SubmissionGroup& group,core::OffscreenMatrix matrix){const auto& g=bindings_.groups.at(&group);if(!g.data&&g.count)throw std::invalid_argument("missing GL mesh quads");for(std::size_t i=0;i<g.count;++i)renderer_.draw(g.data[i],matrix);}
void GlMeshSubmissionHost::after_mesh_draw(core::SubmissionMaterial& m,core::SubmissionGroup& g){diagnostics_(m,g);}
std::int32_t GlMeshSubmissionHost::check_count(){return check_count_();}
} // namespace saga::desktop
