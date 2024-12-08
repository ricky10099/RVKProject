#include "Framework/RVKApp.h"

#include "Framework/keyboard_movement_controller.h"
#include "Framework/Vulkan/RVKBuffer.h"
#include "Framework/Camera.h"
#include "Framework/Vulkan/RenderSystem/simple_render_system.h"
#include "Framework/Vulkan/RenderSystem/point_light_system.h"
#include "Framework/Vulkan/FrameInfo.h"

#include <glm/gtc/constants.hpp>

namespace RVK {

	RVKApp::RVKApp() {
		globalPool =
			RVKDescriptorPool::Builder()
			.SetMaxSets(MAX_FRAMES_IN_FLIGHT)
			.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT)
			.Build();
		LoadGameObjects();
	}

	RVKApp::~RVKApp() {}

	void RVKApp::Run() {
		std::vector<std::unique_ptr<RVKBuffer>> uboBuffers(MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uboBuffers.size(); i++) {
			uboBuffers[i] = std::make_unique<RVKBuffer>(
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			uboBuffers[i]->Map();
		}

		auto globalSetLayout =
			RVKDescriptorSetLayout::Builder()
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.Build();

		std::vector<VkDescriptorSet> globalDescriptorSets(MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->DescriptorInfo();
			RVKDescriptorWriter(*globalSetLayout, *globalPool)
				.WriteBuffer(0, &bufferInfo)
				.Build(globalDescriptorSets[i]);
		}

		SimpleRenderSystem simpleRenderSystem{
			m_rvkRenderer.GetSwapChainRenderPass(),
			globalSetLayout->GetDescriptorSetLayout() };
		PointLightSystem pointLightSystem{
			m_rvkRenderer.GetSwapChainRenderPass(),
			globalSetLayout->GetDescriptorSetLayout() };

		Camera camera{};

		auto viewerObject = GameObject::CreateGameObject();
		viewerObject.m_transform.translation.z = -2.5f;
		KeyboardMovementController cameraController{};

		auto currentTime = std::chrono::high_resolution_clock::now();
		while (!m_rvkWindow.ShouldClose()) {
			glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime =
				std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;

			cameraController.MoveInPlaneXZ(m_rvkWindow.GetGLFWwindow(), frameTime, viewerObject);
			camera.SetViewYXZ(viewerObject.m_transform.translation, viewerObject.m_transform.rotation);

			float aspect = m_rvkRenderer.GetAspectRatio();
			camera.SetPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

			if (auto commandBuffer = m_rvkRenderer.BeginFrame()) {
				int frameIndex = m_rvkRenderer.GetFrameIndex();
				FrameInfo frameInfo{
					frameIndex,
					frameTime,
					commandBuffer,
					camera,
					globalDescriptorSets[frameIndex],
					gameObjects };

				// update
				GlobalUbo ubo{};
				ubo.projection = camera.GetProjection();
				ubo.view = camera.GetView();
				ubo.inverseView = camera.GetInverseView();
				pointLightSystem.Update(frameInfo, ubo);
				uboBuffers[frameIndex]->WriteToBuffer(&ubo);
				uboBuffers[frameIndex]->Flush();

				// render
				m_rvkRenderer.BeginSwapChainRenderPass(commandBuffer);

				// order here matters
				simpleRenderSystem.RenderGameObjects(frameInfo);
				pointLightSystem.Render(frameInfo);

				m_rvkRenderer.EndSwapChainRenderPass(commandBuffer);
				m_rvkRenderer.EndFrame();
			}
		}

		vkDeviceWaitIdle(RVKDevice::s_rvkDevice->GetDevice());
	}

	void RVKApp::LoadGameObjects() {
		std::shared_ptr<Model> lveModel =
			Model::CreateModelFromFile("models/flat_vase.obj");
		auto flatVase = GameObject::CreateGameObject();
		flatVase.m_model = lveModel;
		flatVase.m_transform.translation = { -.5f, .5f, 0.f };
		flatVase.m_transform.scale = { 3.f, 1.5f, 3.f };
		gameObjects.emplace(flatVase.GetId(), std::move(flatVase));

		lveModel = Model::CreateModelFromFile("models/smooth_vase.obj");
		auto smoothVase = GameObject::CreateGameObject();
		smoothVase.m_model = lveModel;
		smoothVase.m_transform.translation = { .5f, .5f, 0.f };
		smoothVase.m_transform.scale = { 3.f, 1.5f, 3.f };
		gameObjects.emplace(smoothVase.GetId(), std::move(smoothVase));

		lveModel = Model::CreateModelFromFile("models/quad.obj");
		auto floor = GameObject::CreateGameObject();
		floor.m_model = lveModel;
		floor.m_transform.translation = { 0.f, .5f, 0.f };
		floor.m_transform.scale = { 3.f, 1.f, 3.f };
		gameObjects.emplace(floor.GetId(), std::move(floor));

		std::vector<glm::vec3> lightColors{
			{1.f, .1f, .1f},
			{.1f, .1f, 1.f},
			{.1f, 1.f, .1f},
			{1.f, 1.f, .1f},
			{.1f, 1.f, 1.f},
			{1.f, 1.f, 1.f}  //
		};

		for (int i = 0; i < lightColors.size(); i++) {
			auto pointLight = GameObject::MakePointLight(0.2f);
			pointLight.m_color = lightColors[i];
			auto rotateLight = glm::rotate(
				glm::mat4(1.f),
				(i * glm::two_pi<float>()) / lightColors.size(),
				{ 0.f, -1.f, 0.f });
			pointLight.m_transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
			gameObjects.emplace(pointLight.GetId(), std::move(pointLight));
		}
	}
}  // namespace RVK
