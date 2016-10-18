/**
 * @file   VKRAIIWrapper.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2016.10.18
 *
 * @brief  Wrappers for Vulkan object to allow RAII patterns.
 */

#pragma once

// ReSharper disable once CppUnusedIncludeDirective
#include "main.h"
#include <vulkan/vulkan.hpp>

namespace vku {

    template<typename ObjectTraits, typename AllocationTraits> class VKRAIIWrapper
    {
    public:
        template<typename... Args>
        explicit VKRAIIWrapper(Args&&... args) : obj(ObjectTraits::template Create<AllocationTraits>(std::forward<Args>(args)...)) {}
        explicit VKRAIIWrapper(typename ObjectTraits::value_type newObj) : obj(newObj) {}
        VKRAIIWrapper(const VKRAIIWrapper&) = delete;
        VKRAIIWrapper& operator=(const VKRAIIWrapper&) = delete;
        VKRAIIWrapper(VKRAIIWrapper&& rhs) : obj(rhs.obj) { rhs.obj = ObjectTraits::null_obj; }
        VKRAIIWrapper& operator=(VKRAIIWrapper&& rhs) { obj = rhs.obj; rhs.obj = ObjectTraits::null_obj; return *this; }
        ~VKRAIIWrapper() { obj = ObjectTraits::template Destroy<AllocationTraits>(obj); }

        operator typename ObjectTraits::value_type() const { return obj; }
        explicit operator bool() const { return ObjectTraits::null_obj != obj; }
        bool operator==(const VKRAIIWrapper& rhs) { return rhs.obj == obj; }

        friend bool operator==(typename ObjectTraits::value_type lhs, const VKRAIIWrapper<ObjectTraits, AllocationTraits>& rhs) { return lhs == rhs.obj; }
        friend bool operator==(const VKRAIIWrapper<ObjectTraits, AllocationTraits>& lhs, typename ObjectTraits::value_type rhs) { return lhs.obj == rhs; }

        typename ObjectTraits::value_type release() { typename ObjectTraits::value_type tmp = obj; obj = ObjectTraits::null_obj; return tmp; }
        void reset(typename ObjectTraits::value_type newObj = ObjectTraits::null_obj) { obj = ObjectTraits::template Destroy<AllocationTraits>(obj); obj = newObj; }
        void swap(VKRAIIWrapper<ObjectTraits, AllocationTraits>& other) { typename ObjectTraits::value_type tmp = obj; obj = other.obj; other.obj = tmp; }

    private:
        typename ObjectTraits::value_type obj;
    };

    struct InstanceObjectTraits
    {
        using value_type = vk::Instance;
        static const value_type null_obj;
        template<typename AllocationTraits, typename... Args>
        static constexpr value_type Create(Args&&... args) { return vk::createInstance(std::forward<Args>(args)..., AllocationTraits::GetAllocCB()); }
        template<typename AllocationTraits>
        static constexpr value_type Destroy(value_type inst) { inst.destroy(AllocationTraits::GetAllocCB()); return null_obj; }
    };

    struct StdAllocationTraits
    {
        static vk::Optional<vk::AllocationCallbacks> GetAllocCB() { return nullptr; }
    };

    using InstanceRAII = VKRAIIWrapper<InstanceObjectTraits, StdAllocationTraits>;

    /*struct ProgramObjectTraits
    {
        using value_type = GLuint;
        static const value_type null_obj = 0;
        static value_type Create() { return OGL_SCALL(glCreateProgram); }
        static value_type Destroy(value_type prog) { OGL_CALL(glDeleteProgram, prog); return null_obj; }
    };

    struct ShaderObjectTraits
    {
        using value_type = GLuint;
        static const value_type null_obj = 0;
        static value_type Create() { return null_obj; }
        static value_type Destroy(value_type shader) { OGL_CALL(glDeleteShader, shader); return null_obj; }
    };

    struct BufferObjectTraits
    {
        using value_type = GLuint;
        static const value_type null_obj = 0;
        static value_type Create() { value_type buffer; OGL_CALL(glGenBuffers, 1, &buffer); return buffer; }
        template<int N> static void Create(std::array<value_type, N>& buffers) { OGL_CALL(glGenBuffers, static_cast<GLsizei>(N), buffers.data()); }
        static value_type Destroy(value_type buffer) { OGL_CALL(glDeleteBuffers, 1, &buffer); return null_obj; }
        template<int N> static void Destroy(std::array<value_type, N>& buffers)
        {
            OGL_CALL(glDeleteBuffers, static_cast<GLsizei>(N), buffers.data());
            for (auto& buffer : buffers) buffer = null_obj;
        }
    };

    struct TextureObjectTraits
    {
        using value_type = GLuint;
        static const value_type null_obj = 0;
        static value_type Create() { value_type texture; OGL_CALL(glGenTextures, 1, &texture); return texture; }
        template<int N> static void Create(std::array<value_type, N>& textures) { OGL_CALL(glGenTextures, static_cast<GLsizei>(N), textures.data()); }
        static value_type Destroy(value_type texture) { OGL_CALL(glDeleteTextures, 1, &texture); return null_obj; }
        template<int N> static void Destroy(std::array<value_type, N>& textures)
        {
            OGL_CALL(glDeleteTextures, static_cast<GLsizei>(N), textures.data());
            for (auto& texture : textures) texture = null_obj;
        }
    };

    struct FramebufferObjectTraits
    {
        using value_type = GLuint;
        static const value_type null_obj = 0;
        static value_type Create() { value_type fbo; OGL_CALL(glGenFramebuffers, 1, &fbo); return fbo; }
        template<int N> static void Create(std::array<value_type, N>& fbos) { OGL_CALL(glGenFramebuffers, static_cast<GLsizei>(N), fbos.data()); }
        static value_type Destroy(value_type fbo) { OGL_CALL(glDeleteFramebuffers, 1, &fbo); return null_obj; }
        template<int N> static void Destroy(std::array<value_type, N>& fbos)
        {
            OGL_CALL(glDeleteFramebuffers, static_cast<GLsizei>(N), fbos.data());
            for (auto& fbo : fbos) fbo = null_obj;
        }
    };

    struct RenderbufferObjectTraits
    {
        using value_type = GLuint;
        static const value_type null_obj = 0;
        static value_type Create() { value_type rbo; OGL_CALL(glGenRenderbuffers, 1, &rbo); return rbo; }
        template<int N> static void Create(std::array<value_type, N>& rbos) { OGL_CALL(glGenRenderbuffers, static_cast<GLsizei>(N), rbos.data()); }
        static value_type Destroy(value_type rbo) { OGL_CALL(glDeleteRenderbuffers, 1, &rbo); return null_obj; }
        template<int N> static void Destroy(std::array<value_type, N>& rbos)
        {
            OGL_CALL(glDeleteRenderbuffers, static_cast<GLsizei>(N), rbos.data());
            for (auto& rbo : rbos) rbo = null_obj;
        }
    };

    struct VertexArrayObjectTraits
    {
        using value_type = GLuint;
        static const value_type null_obj = 0;
        static value_type Create() { value_type vao; OGL_CALL(glGenVertexArrays, 1, &vao); return vao; }
        template<int N> static void Create(std::array<value_type, N>& vaos) { OGL_CALL(glGenVertexArrays, static_cast<GLsizei>(N), vaos.data()); }
        static value_type Destroy(value_type vao) { OGL_CALL(glDeleteVertexArrays, 1, &vao); return null_obj; }
        template<int N> static void Destroy(std::array<value_type, N>& vaos)
        {
            OGL_CALL(glDeleteVertexArrays, static_cast<GLsizei>(N), vaos.data());
            for (auto& vao : vaos) vao = null_obj;
        }
    };*/

    /*using ProgramRAII = VKRAIIWrapper<ProgramObjectTraits, 1>;
    using ShaderRAII = VKRAIIWrapper<ShaderObjectTraits, 1>;
    template<int N> using BuffersRAII = OpenGLRAIIWrapper<BufferObjectTraits, N>;
    using BufferRAII = VKRAIIWrapper<BufferObjectTraits, 1>;
    template<int N> using TexuturesRAII = OpenGLRAIIWrapper<TextureObjectTraits, N>;
    using TextureRAII = VKRAIIWrapper<TextureObjectTraits, 1>;
    template<int N> using FramebuffersRAII = OpenGLRAIIWrapper<FramebufferObjectTraits, N>;
    using FramebufferRAII = VKRAIIWrapper<FramebufferObjectTraits, 1>;
    template<int N> using RenderbuffersRAII = OpenGLRAIIWrapper<RenderbufferObjectTraits, N>;
    using RenderbufferRAII = VKRAIIWrapper<RenderbufferObjectTraits, 1>;
    template<int N> using VertexArraysRAII = OpenGLRAIIWrapper<VertexArrayObjectTraits, N>;
    using VertexArrayRAII = VKRAIIWrapper<VertexArrayObjectTraits, 1>;*/
}
