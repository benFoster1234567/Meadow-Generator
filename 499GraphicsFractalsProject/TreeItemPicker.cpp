#include "TreeItemPicker.h"
#include <any>

TreeItemPicker::TreeDangler TreeItemPicker::addBox(const glm::mat4& transform, const glm::vec3& halfBounds, float mass)
{
	Transform t{};
	t.set(transform);

	RenderedObject obj{ m_mesh, m_shader, m_material };
	obj.getTransform().set(t.matrix());
	auto rigidBody = m_physics.createRigidBodyBox(halfBounds, t, mass, false, false);
	obj.attachRigidbody(rigidBody);

	TreeDangler d = { .m_object = obj, .isFree = false };

	return d;
}

TreeItemPicker::TreeDangler TreeItemPicker::addItem(const glm::mat4& transform, const Collider& collider, float mass)
{

	Transform t{};
	t.set(transform);
	
	m_collider = collider;

	RenderedObject obj{ m_mesh, m_shader, m_material };
	obj.getTransform().set(t.matrix());
	BtPhysicsEngine::RigidBodyID rigidBody = m_physics.createRigidBody(m_collider, t, mass, false, false);

	obj.attachRigidbody(rigidBody);

	TreeDangler d = { .m_object = obj, .isFree = false };

	return d;
}

void TreeItemPicker::freeObject(const unsigned int& objectId)
{
	if (!m_enabled) return;
	if (m_objects[objectId].isFree) return;

	m_objects[objectId].isFree = true;
	auto rib = m_objects[objectId].m_object.getRigidbodyId();
	m_physics.activateRigidbody(rib, m_objects[objectId].m_object.getTransform());
}

void TreeItemPicker::draw(const Camera& camera, std::weak_ptr<Shader> shader, uint8_t flags, bool toPicker)
{
	if (!m_enabled) return;

	camera.setUniformPos("viewPosition", shader);
	camera.setUniformProj("projection", shader);
	camera.setUniformView("view", shader);

	auto lock = shader.lock();
	if (!lock) return;
	lock->useShader();
	for (int i = 0; i < m_objects.size(); ++i)
	{
		if (toPicker)
		{
			lock->setUInt("index", static_cast<unsigned int>(i + 1));
			m_objects[i].m_object.draw(lock);
		}
		else
		{
			m_objects[i].m_object.draw(0);
		}
	}
}

void TreeItemPicker::initialize(unsigned int width, unsigned int height, std::shared_ptr<GpuMesh> mesh, Material material, std::shared_ptr<Shader> shader, std::shared_ptr<Shader> pickerShader, const std::vector<glm::mat4>& initialTransforms, const Collider& collider, float mass)
{
	if (m_initialized) return;

	m_material = material;
	m_mesh = std::move(mesh);
	m_shader = shader;
	m_pickerShader = std::move(pickerShader);
	m_mass = mass;

	if (!m_mesh->isInitialized()) m_mesh->initialize();
	m_pickingTexture.initialize(width, height);

	for (const auto& t : initialTransforms)
	{
		m_objects.push_back(addItem(t, collider, m_mass));
		
	}

	m_initialized = true;
}

void TreeItemPicker::changePositions(const std::vector<glm::mat4>& transforms)
{
	m_physics.clear();
	// TODO: re-create kinematic bodies here

	std::deque<TreeDangler> newObjects;
	for (const auto& m : transforms)
	{
		glm::vec3 scale, translation, skew;
		glm::quat rotation;
		glm::vec4 perspective;
		glm::decompose(m, scale, rotation, translation, skew, perspective);
		translation += glm::vec3(0, -.5f, 0);
		newObjects.push_back(addItem(glm::translate(translation) * glm::scale(glm::mat4(1), m_scale),
			m_collider, m_mass));
	}
	m_objects = std::move(newObjects);
}

void TreeItemPicker::onClick(int x, int y, const Camera& camera)
{
	if (!m_enabled) return;
	m_pickingTexture.enableWriting();
	GLuint clearColor[4] = { 0u, 0u, 0u, 0u };
	glClearBufferuiv(GL_COLOR, 0, clearColor);
	draw(camera, m_pickerShader, static_cast<uint8_t>(RenderFlag::DontSendMat) | static_cast<uint8_t>(RenderFlag::DontSendMaps), true);
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	m_pickingTexture.disableWriting();

	auto p = m_pickingTexture.readPixel(x, viewport[3] - y);

	std::cout << "item " << p.objectID << " picked!\n";
	if (p.objectID > 0 && p.objectID <= m_objects.size())
	{
		freeObject(p.objectID - 1);
	}

}
