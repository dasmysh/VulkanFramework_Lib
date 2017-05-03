/**
 * @file   Renderer.h
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.04.15
 *
 * @brief  Abstract class for all renderers in the engine.
 */

#pragma once

#include <vector>
/*
#include "engine/components/render_component.h"
#include "engine/gfx/gpu_program.h"
#include "engine/gfx/renderable.h"

#include <glm/mat4x4.hpp>

namespace get {

  class Camera;
  class Framebuffer;
  using Scene = std::vector<std::uint32_t>;

  class Renderer
  {
  public:
    Renderer(GPUProgram* program) : program_{program} {}

    void Init(Engine* engine) { engine_ = engine; }

    void Shutdown() {}

    void RecompileProgram()
    {
      program_->Recompile();
      UpdateUniformLocations();
    }

    virtual void Draw(const Framebuffer&, const Camera&, const Scene&) = 0;
    void RegisterSceneObjects(const std::vector<std::uint32_t>& sceneObjects);

  protected:
    template<typename VertexType>
    void RegisterGameObject(Handle game_object_handle);

    // TODO replace
    virtual void DrawNode(const Renderable& render_component,
                          const glm::mat4& model_matrix,
                          std::size_t node_id) = 0;

    // TODO replace
    virtual void DrawNodePart(const Renderable& render_component,
                              const glm::mat4& model_matrix,
                              std::size_t node_id, std::size_t part_id) = 0;

    virtual void UpdateUniformLocations() = 0;

    GPUProgram* program_;
    Engine* engine_;

    std::vector<GLint> uniform_locations_;
  };

  template<typename VertexType>
  void Renderer::RegisterGameObject(Handle game_object_handle)
  {
    const auto& game_object =
        engine_->game_object_manager_->FromHandle(game_object_handle);

    if (game_object.HasComponent<RenderComponent>()) {
      auto render_component = game_object.GetComponent<RenderComponent>();
      auto renderable = render_component->GetRenderable();
      renderable->RegisterVertexFormat<VertexType>();
    }
  }

} // namespace get
*/
