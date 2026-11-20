#include "Renderer.h"

ShaderID Renderer::addShader(std::string_view vertexFile, std::string_view fragFile)
{
    ShaderID id = m_shaders.size();
    m_shaders.emplace_back();
    m_renderPasses.emplace_back();
    m_renderPasses.back().shader = id;
    m_shaders.back().loadShader(vertexFile, fragFile);
    return id;
}

RenderableID Renderer::addRenderable(Renderable&& renderable)
{
    RenderableID id = m_renderables.size();
    m_renderables.emplace_back(std::move(renderable));
    m_renderables.back().id = id;
    return id;
}

void Renderer::attachRenderableToShaderRenderPass(ShaderID shader, RenderableID renderable)
{
    m_renderPasses[shader].renderables.push_back(renderable);
}

MeshLoaderID Renderer::loadMesh(const Mesh& mesh)
{
    MeshLoaderID id = m_nextMesh;
    m_nextMesh++;
    m_meshLoaders.insert(id, MeshLoader(mesh));
    return id;
}

MeshLoaderID Renderer::loadMeshInstanced(const Mesh& mesh)
{
    MeshLoaderID id = m_nextMesh;
    m_nextMesh++;
    m_meshLoadersInstanced.insert(id, MeshLoaderInstanced(mesh));
    return id;
}

void Renderer::changeMesh(MeshLoaderID id, Mesh mesh)
{
    if (m_meshLoaders.contains(id))
    {
        m_meshLoaders.get(id).changeMesh(std::move(mesh));
        return;
    }

    if (m_meshLoadersInstanced.contains(id))
    {
        m_meshLoadersInstanced.get(id).changeMesh(std::move(mesh));
        return;
    }

    throw std::out_of_range("Mesh loader ID does not exist");
}

void Renderer::changeInstances(MeshLoaderID id, const std::vector<glm::mat4>& instances)
{
    if (!m_meshLoadersInstanced.contains(id))
    {
        throw std::out_of_range("Instanced mesh loader ID does not exist");
    }

    m_meshLoadersInstanced.get(id).changeInstances(instances);
}

void Renderer::initialize()
{
    for (auto& m : m_meshLoaders)
    {
        m.initialize();
    }

    for (auto& m : m_meshLoadersInstanced)
    {
        m.initialize();
    }
}

void Renderer::draw(Camera& camera)
{
    for (size_t rp = 0; rp < m_renderPasses.size(); ++rp)
    {
        const RenderPass& renderPass = m_renderPasses[rp];
        auto& shader = m_shaders[renderPass.shader];
        shader.useShader();

        shader.setMat4("view", camera.view());
        shader.setMat4("projection", camera.projection());
        shader.setVec3("viewPosition", camera.eye());

        for (size_t rid = 0; rid < renderPass.renderables.size(); ++rid)
        {
            Renderable& renderable = m_renderables[renderPass.renderables[rid]];
            
            renderable.setupDraw();

            shader.setMat4("model", renderable.transform.matrix());
            
            if (renderPass.sendMaterials)
            {
                shader.setMaterial(renderable.material, renderPass.sendMatMaps);
                shader.setMat3("texture", renderable.textureScale);
            }

            if (renderable.isInstanced)
            {
                m_meshLoadersInstanced.get(renderable.mesh).draw();
            }

            else
            {
                m_meshLoaders.get(renderable.mesh).draw();
            }
        }
    }

}
