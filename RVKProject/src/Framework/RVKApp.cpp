#include "Framework/RVKApp.h"

#include <glm/gtc/constants.hpp>

#include "Framework/keyboard_movement_controller.h"
#include "Framework/Vulkan/RVKBuffer.h"
#include "Framework/Camera.h"
#include "Framework/Vulkan/RenderSystem/simple_render_system.h"
#include "Framework/Vulkan/RenderSystem/entity_render_system.h"
#include "Framework/Vulkan/RenderSystem/point_light_system.h"
#include "Framework/Vulkan/RenderSystem/entity_point_light_system.h"
#include "Framework/Vulkan/FrameInfo.h"
#include "Framework/Component.h"

namespace RVK {

	RVKApp::RVKApp() {
		globalPool =
			RVKDescriptorPool::Builder()
			.SetMaxSets(MAX_FRAMES_IN_FLIGHT)
			.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT)
			.Build();

		/////////////////////////////////////////////////////////////////
		m_pFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_defaultAllocator, m_defaultErrorCallback);
		m_pPvd = physx::PxCreatePvd(*m_pFoundation);
		physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
		m_pPvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
		m_pPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_pFoundation, physx::PxTolerancesScale(), true, m_pPvd);
		m_pDispatcher = physx::PxDefaultCpuDispatcherCreate(8);
		physx::PxSceneDesc scene_desc(m_pPhysics->getTolerancesScale());
		scene_desc.gravity = physx::PxVec3(0, -9, 0);
		scene_desc.filterShader = physx::PxDefaultSimulationFilterShader;
		scene_desc.cpuDispatcher = m_pDispatcher;
		m_pScene = m_pPhysics->createScene(scene_desc);
		physx::PxPvdSceneClient* pvd_client = m_pScene->getScenePvdClient();
		pvd_client->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvd_client->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvd_client->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);

		// create simulation
		m_pMaterial = m_pPhysics->createMaterial(0.0f, 0.0f, 0.0f);
		physx::PxRigidStatic* groundPlane = PxCreatePlane(*m_pPhysics, physx::PxPlane(0, 1, 0, 50), *m_pMaterial);
		m_pScene->addActor(*groundPlane);
		/////////////////////////////////////////////////////////////////

		LoadScene(std::make_unique<GameScene>());
		LoadGameObjects();
	}

	RVKApp::~RVKApp() {
		PxCloseExtensions();
		m_pScene->release();
		m_pDispatcher->release();
		m_pPhysics->release();
		if (m_pPvd) {
			m_pPvd->disconnect();
			physx::PxPvdTransport* transport = m_pPvd->getTransport();
			m_pPvd->release();
			transport->release();
		}
		m_pFoundation->release();
	}

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

		//SimpleRenderSystem simpleRenderSystem{
		//	m_rvkRenderer.GetSwapChainRenderPass(),
		//	globalSetLayout->GetDescriptorSetLayout() };
		EntityRenderSystem entityRenderSystem{
			m_rvkRenderer.GetSwapChainRenderPass(),
			globalSetLayout->GetDescriptorSetLayout() };

		PointLightSystem pointLightSystem{
			m_rvkRenderer.GetSwapChainRenderPass(),
			globalSetLayout->GetDescriptorSetLayout() };

		EntityPointLightSystem entityPointLightSystem{
			m_rvkRenderer.GetSwapChainRenderPass(),
			globalSetLayout->GetDescriptorSetLayout() };

		//Camera camera{};

		auto viewerObject = GameObject::CreateGameObject();
		viewerObject.m_transform.translation = { 0.0f, 1.0f, -2.5f };
		KeyboardMovementController cameraController{};

		auto currentTime = std::chrono::high_resolution_clock::now();

		m_test = m_currentScene->CreateEntity("test");
		m_test.AddComponent<Components::Transform>(glm::vec3(0.f, 0.f, 0.f))
			.scale = {.1f, 0.1f, 0.1f};
		m_test.AddComponent<Components::Mesh>("models/Male.obj");

		m_testLight = m_currentScene->CreateEntity("testLight");
		m_testLight.AddComponent<Components::Transform>(glm::vec3(0.f, 0.f, 0.f));
		m_testLight.AddComponent<Components::PointLight>(glm::vec3(1.f, 0.f, 0.f), 0.1f, 0.1f );
		//float aspect = m_rvkRenderer.GetAspectRatio();

		//for (auto [entity, cam, transform] : 
		//		m_currentScene->m_entityRoot.view<Components::CameraComponent, Components::Transform>().each()) {
		//		if (cam.currentCamera) {
		//			cam.camera.SetPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);
		//		}
		//	}

		while (!m_rvkWindow.ShouldClose()) {
			glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime =
				std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;

			cameraController.MoveInPlaneXZ(m_rvkWindow.GetGLFWwindow(), frameTime, m_test, viewerObject);
			//camera.SetViewYXZ(viewerObject.m_transform.translation, viewerObject.m_transform.rotation);

			float aspect = m_rvkRenderer.GetAspectRatio();
			for (auto [entity, cam, transform] : 
				m_currentScene->m_entityRoot.view<Components::Camera, Components::Transform>().each()) {
				if (cam.currentCamera) {
					const glm::vec3 forwardDir{ 0.f, 0.f, 1.0f };
					const glm::vec3 rightDir{ 1, 0.f, 0 };
					const glm::vec3 upDir{ 0.f, -1.f, 0.f };

					glm::vec3 moveDir{ 0.f };
					if (glfwGetKey(m_rvkWindow.GetGLFWwindow(), GLFW_KEY_UP) == GLFW_PRESS) moveDir += forwardDir;
					if (glfwGetKey(m_rvkWindow.GetGLFWwindow(), GLFW_KEY_DOWN) == GLFW_PRESS) moveDir -= forwardDir;
					if (glfwGetKey(m_rvkWindow.GetGLFWwindow(), GLFW_KEY_LEFT) == GLFW_PRESS) moveDir -= rightDir;
					if (glfwGetKey(m_rvkWindow.GetGLFWwindow(), GLFW_KEY_RIGHT) == GLFW_PRESS) moveDir += rightDir;

					if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
						transform.position += 3.0f * frameTime * glm::normalize(moveDir);
					}

					cam.camera.SetViewYXZ(transform.position, transform.rotation);
					cam.camera.SetPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);
				}
			}
			/*physx::PxTransform t{ reinterpret_cast<const physx::PxVec3&>(gameObjects.at(0).m_transform.translation + glm::vec3(0.f, 1.5f, 0.f)) };
			m_pBody->setGlobalPose(t);*/

			//m_pScene->simulate(frameTime);
			//m_pScene->fetchResults(true);

			//gameObjects.at(0).m_transform.translation = reinterpret_cast<const glm::vec3&>(m_pBody->getGlobalPose().p) - glm::vec3(0.f, 1.5f, 0.f);

			//camera.SetPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);
			//Camera& scenecamera = m_currentScene->MainCamera();
			if (auto commandBuffer = m_rvkRenderer.BeginFrame()) {
				int frameIndex = m_rvkRenderer.GetFrameIndex();
				FrameInfo frameInfo{
					frameIndex,
					frameTime,
					commandBuffer,
					//camera,
					globalDescriptorSets[frameIndex],
					gameObjects };

				// update
				GlobalUbo ubo{};
				for (auto [entity, cam] :
					m_currentScene->m_entityRoot.view<Components::Camera>().each()) {
					if (cam.currentCamera) {
						ubo.projection = cam.camera.GetProjection();
						ubo.view = cam.camera.GetView();
					}
				}
				//ubo.projection = camera.GetProjection();
				//ubo.view = camera.GetView();
				//ubo.inverseView = camera.GetInverseView();
				//pointLightSystem.Update(frameInfo, ubo);
				entityPointLightSystem.Update(frameInfo, ubo, m_currentScene->m_entityRoot);
				uboBuffers[frameIndex]->WriteToBuffer(&ubo);
				uboBuffers[frameIndex]->Flush();

				// render
				m_rvkRenderer.BeginSwapChainRenderPass(commandBuffer);

				// order here matters
				//simpleRenderSystem.RenderGameObjects(frameInfo);
				entityRenderSystem.RenderEntities(frameInfo, m_currentScene->m_entityRoot);
				entityPointLightSystem.Render(frameInfo, m_currentScene->m_entityRoot);
				//pointLightSystem.Render(frameInfo);

				m_rvkRenderer.EndSwapChainRenderPass(commandBuffer);
				m_rvkRenderer.EndFrame();
			}
		}

		vkDeviceWaitIdle(RVKDevice::s_rvkDevice->GetDevice());
	}

	void RVKApp::LoadGameObjects() {
		//std::shared_ptr<Model> model =
		//	Model::CreateModelFromFile("models/Male.obj");
		//auto male = GameObject::CreateGameObject();
		//male.m_model = model;
		//male.m_transform.translation = { -.5f, 1.5f, 0.f };
		//male.m_transform.scale = { .1f, .1f, .1f };
		//gameObjects.emplace(male.GetId(), std::move(male));
		//physx::PxShape* shape = m_pPhysics->createShape(physx::PxCapsuleGeometry(0.5f, 1.0f), *m_pMaterial);
		//{
		//	physx::PxTransform localTm(physx::PxVec3(-.5f, 1.5f, 0.f));
		//	physx::PxTransform relativePose(physx::PxQuat(physx::PxHalfPi, physx::PxVec3(0, 0, 1)));
		//	m_pBody = m_pPhysics->createRigidDynamic(localTm);
		//	shape->setLocalPose(relativePose);
		//	m_pBody->attachShape(*shape);
		//	physx::PxRigidBodyExt::updateMassAndInertia(*m_pBody, 10.0f);
		//	m_pScene->addActor(*m_pBody);
		//	//m_pBody->setRigidDynamicLockFlags(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y);
		//}
		//shape = m_pPhysics->createShape(physx::PxBoxGeometry(1.f, 1.0f, 1.0f), *m_pMaterial);
		//{
		//	physx::PxTransform localTm(physx::PxVec3(-.5f, .5f, 2.f));
		//	physx::PxRigidStatic* b= m_pPhysics->createRigidStatic(localTm);
		//	b->attachShape(*shape);
		//	m_pScene->addActor(*b);
		//}

		//model = Model::CreateModelFromFile("models/smooth_vase.obj");
		//auto smoothVase = GameObject::CreateGameObject();
		//smoothVase.m_model = model;
		//smoothVase.m_transform.translation = { .5f, 1.5f, 0.f };
		//smoothVase.m_transform.scale = { 3.f, 1.5f, 3.f };
		//gameObjects.emplace(smoothVase.GetId(), std::move(smoothVase));

		//model = Model::CreateModelFromFile("models/quad.obj");
		//auto floor = GameObject::CreateGameObject();
		//floor.m_model = model;
		//floor.m_transform.translation = { 2.f, .5f, 2.f };
		//floor.m_transform.scale = { 3.f, 1.f, 3.f };
		//gameObjects.emplace(floor.GetId(), std::move(floor));
		//shape = m_pPhysics->createShape(physx::PxBoxGeometry(3.0f, 0.001f, 3.0f), *m_pMaterial);
		//{
		//	physx::PxTransform localTm(physx::PxVec3(2.0f, .5f, 2.f));
		//	m_pFloor = m_pPhysics->createRigidStatic(localTm);
		//	m_pFloor->attachShape(*shape);
		//	m_pScene->addActor(*m_pFloor);
		//}

		std::vector<glm::vec3> lightColors{
			{1.f, .1f, .1f},
			{.1f, .1f, 1.f},
			{.1f, 1.f, .1f},
			{1.f, 1.f, .1f},
			{.1f, 1.f, 1.f},
			{1.f, 1.f, 1.f},
		};

		//for (int i = 0; i < lightColors.size(); i++) {
		//	auto pointLight = GameObject::MakePointLight(0.2f);
		//	pointLight.m_color = lightColors[i];
		//	auto rotateLight = glm::rotate(
		//		glm::mat4(1.f),
		//		(i * glm::two_pi<float>()) / lightColors.size(),
		//		{ 0.f, 1.f, 0.f });
		//	pointLight.m_transform.translation = glm::vec3(rotateLight * glm::vec4(1.f, 1.f, 1.f, 1.f));
		//	gameObjects.emplace(pointLight.GetId(), std::move(pointLight));
		//}

		for (int i = 0; i < lightColors.size(); i++) {
			auto pointLight = m_currentScene->CreateEntity("Point Light "+ i);
			pointLight.AddComponent<Components::PointLight>(lightColors[i]);
			auto rotateLight = glm::rotate(
				glm::mat4(1.f),
				(i * glm::two_pi<float>()) / lightColors.size(),
				{ 0.f, 1.f, 0.f });
			pointLight.AddComponent<Components::Transform>(glm::vec3(rotateLight * glm::vec4(1.f, 1.f, 1.f, 1.f)));
		}
	}

	void RVKApp::LoadScene(std::unique_ptr<GameScene> scene) {
		m_currentScene = std::move(scene);
	}
}  // namespace RVK
