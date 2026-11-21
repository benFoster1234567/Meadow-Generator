#include "pch.h"
#include "CppUnitTest.h"
#include "../RendererLibrary/Renderer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Tests
{
	TEST_CLASS(RendererTests)
	{
	public:
		TEST_METHOD(AddRenderableAssignsId)
		{
			Renderer renderer;
			Renderable renderable;

			const RenderableID id = renderer.addRenderable(std::move(renderable));

			Assert::AreEqual<RenderableID>(0, id);
			Assert::AreEqual<RenderableID>(id, renderer.addRenderable(Renderable{}));
		}

		TEST_METHOD(SetTransformUpdatesRenderableTransform)
		{
			Renderer renderer;
			const RenderableID id = renderer.addRenderable(Renderable{});
			Transform transform;
			transform.setPosition({ 1.0f, 2.0f, 3.0f });

			renderer.setTransform(id, transform);

			const glm::vec3& position = renderer.getTransform(id).getPosition();
			Assert::AreEqual(1.0f, position.x);
			Assert::AreEqual(2.0f, position.y);
			Assert::AreEqual(3.0f, position.z);
		}
	};
}
