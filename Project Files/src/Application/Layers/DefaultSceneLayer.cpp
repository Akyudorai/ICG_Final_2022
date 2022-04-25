#include "DefaultSceneLayer.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include <GLM/gtc/random.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>

// Graphics
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/Textures/Texture2DArray.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"
#include "Graphics/Framebuffer.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Gameplay/Components/Light.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"
#include "Gameplay/Components/PlayerController.h"
#include "Gameplay/Components/EnemyController.h"
#include "Gameplay/Components/Projectile.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/Texture1D.h"
#include "Application/Layers/ImGuiDebugLayer.h"
#include "Application/Windows/DebugWindow.h"
#include "Gameplay/Components/ShadowCamera.h"
#include "Gameplay/Components/ShipMoveBehaviour.h"

DefaultSceneLayer::DefaultSceneLayer() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad;
}

DefaultSceneLayer::~DefaultSceneLayer() = default;

void DefaultSceneLayer::OnAppLoad(const nlohmann::json& config) {
	_CreateScene();
}

void DefaultSceneLayer::_CreateScene()
{
	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get();

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	} else {
		 

#pragma region Shaders

		// Basic gbuffer generation with no vertex manipulation
		ShaderProgram::Sptr deferredForward = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});
		deferredForward->SetDebugName("Deferred - GBuffer Generation");  

		// Our foliage shader which manipulates the vertices of the mesh
		ShaderProgram::Sptr foliageShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});  
		foliageShader->SetDebugName("Foliage");   

		// This shader handles our multitexturing example
		ShaderProgram::Sptr multiTextureShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/vert_multitextured.glsl" },  
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_multitextured.glsl" }
		});
		multiTextureShader->SetDebugName("Multitexturing"); 

		// This shader handles our displacement mapping example
		ShaderProgram::Sptr displacementShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});
		displacementShader->SetDebugName("Displacement Mapping");

		// This shader handles our cel shading example
		ShaderProgram::Sptr celShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/cel_shader.glsl" }
		});
		celShader->SetDebugName("Cel Shader");

		// Basic gbuffer generation with no vertex manipulation
		ShaderProgram::Sptr customDefault = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/custom_shader.glsl" }
		});
		deferredForward->SetDebugName("Custom Default");

#pragma endregion

#pragma region Meshes

		// Load in the meshes
		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		MeshResource::Sptr shipMesh   = ResourceManager::CreateAsset<MeshResource>("fenrir.obj");

#pragma endregion

#pragma region Textures

		// Load in some textures
		Texture2D::Sptr    boxTexture   = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr    boxSpec      = ResourceManager::CreateAsset<Texture2D>("textures/box-specular.png");
		Texture2D::Sptr    monkeyTex    = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		Texture2D::Sptr    leafTex      = ResourceManager::CreateAsset<Texture2D>("textures/leaves.png");

		Texture2D::Sptr		roadTexture = ResourceManager::CreateAsset<Texture2D>("textures/road.png");
		Texture2D::Sptr		sidewalkTexture = ResourceManager::CreateAsset<Texture2D>("textures/sidewalk.png");
		Texture2D::Sptr		brickTexture = ResourceManager::CreateAsset<Texture2D>("textures/brick.png");
		leafTex->SetMinFilter(MinFilter::Nearest);
		leafTex->SetMagFilter(MagFilter::Nearest);

		// Load some images for drag n' drop
		ResourceManager::CreateAsset<Texture2D>("textures/flashlight.png");
		Texture2D::Sptr  flashlight2 = ResourceManager::CreateAsset<Texture2D>("textures/flashlight-2.png");
		ResourceManager::CreateAsset<Texture2D>("textures/light_projection.png");

		Texture2DArray::Sptr particleTex = ResourceManager::CreateAsset<Texture2DArray>("textures/particles.png", 2, 2);

		//DebugWindow::Sptr debugWindow = app.GetLayer<ImGuiDebugLayer>()->GetWindow<DebugWindow>();

#pragma endregion

#pragma region Basic Texture Creation
		Texture2DDescription singlePixelDescriptor;
		singlePixelDescriptor.Width = singlePixelDescriptor.Height = 1;
		singlePixelDescriptor.Format = InternalFormat::RGB8;

		float normalMapDefaultData[3] = { 0.5f, 0.5f, 1.0f };
		Texture2D::Sptr normalMapDefault = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		normalMapDefault->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, normalMapDefaultData);

		float solidGrey[3] = { 0.5f, 0.5f, 0.5f };
		float solidBlack[3] = { 0.0f, 0.0f, 0.0f };
		float solidWhite[3] = { 1.0f, 1.0f, 1.0f };

		Texture2D::Sptr solidBlackTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidBlackTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidBlack);

		Texture2D::Sptr solidGreyTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidGreyTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidGrey);

		Texture2D::Sptr solidWhiteTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidWhiteTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidWhite);

#pragma endregion 

		// Loading in a 1D LUT
		Texture1D::Sptr toonLut = ResourceManager::CreateAsset<Texture1D>("luts/toon-1D.png"); 
		toonLut->SetWrap(WrapMode::ClampToEdge);

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/ocean/ocean.jpg");
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" } 
		});
		  
		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>();  

		// Setting up our enviroment map
		scene->SetSkyboxTexture(testCubemap); 
		scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up 
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Loading in a color lookup table
		Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/cool.CUBE");   

		// Configure the color correction LUT
		scene->SetColorLUT(lut);

#pragma region Materials

		// Create our materials
		// This will be our box material, with no environment reflections
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			boxMaterial->Name = "Box";
			boxMaterial->Set("u_Material.AlbedoMap", boxTexture);
			boxMaterial->Set("u_Material.Shininess", 0.1f);
			boxMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr brickMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			brickMaterial->Name = "Brick";
			brickMaterial->Set("u_Material.AlbedoMap", brickTexture);
			brickMaterial->Set("u_Material.Shininess", 0.1f);
			brickMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr roadMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			roadMaterial->Name = "Road";
			roadMaterial->Set("u_Material.AlbedoMap", roadTexture);
			roadMaterial->Set("u_Material.Shininess", 0.1f);
			roadMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr sidewalkMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			sidewalkMaterial->Name = "Sidewalk";
			sidewalkMaterial->Set("u_Material.AlbedoMap", sidewalkTexture);
			sidewalkMaterial->Set("u_Material.Shininess", 0.1f);
			sidewalkMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr customMaterial = ResourceManager::CreateAsset<Material>(customDefault);
		{
			customMaterial->Name = "Monkey";
			customMaterial->Set("u_Material.AlbedoMap", monkeyTex);
			customMaterial->Set("u_Material.NormalMap", normalMapDefault);
			customMaterial->Set("u_Material.Shininess", 0.5f);

			customMaterial->Set("u_Light.ToggleAmbience", true);
			customMaterial->Set("u_Light.AmbienceStrength", 0.5f);
			customMaterial->Set("u_Light.ToggleDiffuse", true);
			customMaterial->Set("u_Light.ToggleSpecular", true);
			customMaterial->Set("u_Light.SpecularStrength", 1.0f);
			customMaterial->Set("u_Light.ToggleInversion", false);

			customMaterial->Set("u_Light.Color", glm::vec3(1, 1, 1));
			customMaterial->Set("u_Light.Position", glm::vec3(0, -3, 5));

			customMaterial->Set("u_Effect.Enabled", false);
			customMaterial->Set("u_Effect.ColorMod", glm::vec3(1.0f, 0.0f, 0.0f));
			customMaterial->Set("u_Effect.MinimumColor", 0.5f);
		}

		// This will be the reflective material, we'll make the whole thing 50% reflective
		Material::Sptr testMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			testMaterial->Name = "Box-Specular";
			testMaterial->Set("u_Material.AlbedoMap", boxTexture);
			testMaterial->Set("u_Material.Specular", boxSpec);
			testMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		// Our foliage vertex shader material 
		Material::Sptr foliageMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			foliageMaterial->Name = "Foliage Shader";
			foliageMaterial->Set("u_Material.AlbedoMap", leafTex);
			foliageMaterial->Set("u_Material.Shininess", 0.1f);
			foliageMaterial->Set("u_Material.DiscardThreshold", 0.1f);
			foliageMaterial->Set("u_Material.NormalMap", normalMapDefault);

			foliageMaterial->Set("u_WindDirection", glm::vec3(1.0f, 1.0f, 0.0f));
			foliageMaterial->Set("u_WindStrength", 0.5f);
			foliageMaterial->Set("u_VerticalScale", 1.0f);
			foliageMaterial->Set("u_WindSpeed", 1.0f);
		}

		// Our toon shader material
		Material::Sptr toonMaterial = ResourceManager::CreateAsset<Material>(celShader);
		{
			toonMaterial->Name = "Toon";
			toonMaterial->Set("u_Material.AlbedoMap", boxTexture);
			toonMaterial->Set("u_Material.NormalMap", normalMapDefault);
			toonMaterial->Set("s_ToonTerm", toonLut);
			toonMaterial->Set("u_Material.Shininess", 0.1f);
			toonMaterial->Set("u_Material.Steps", 8);
		}


		Material::Sptr displacementTest = ResourceManager::CreateAsset<Material>(displacementShader);
		{
			Texture2D::Sptr displacementMap = ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png");
			Texture2D::Sptr normalMap = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			displacementTest->Name = "Displacement Map";
			displacementTest->Set("u_Material.AlbedoMap", diffuseMap);
			displacementTest->Set("u_Material.NormalMap", normalMap);
			displacementTest->Set("s_Heightmap", displacementMap);
			displacementTest->Set("u_Material.Shininess", 0.5f);
			displacementTest->Set("u_Scale", 0.1f);
		}

		Material::Sptr grey = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			grey->Name = "Grey";
			grey->Set("u_Material.AlbedoMap", solidGreyTex);
			grey->Set("u_Material.Specular", solidBlackTex);
			grey->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr polka = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			polka->Name = "Polka";
			polka->Set("u_Material.AlbedoMap", ResourceManager::CreateAsset<Texture2D>("textures/polka.png"));
			polka->Set("u_Material.Specular", solidBlackTex);
			polka->Set("u_Material.NormalMap", normalMapDefault);
			polka->Set("u_Material.EmissiveMap", ResourceManager::CreateAsset<Texture2D>("textures/polka.png"));
		}

		Material::Sptr whiteBrick = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			whiteBrick->Name = "White Bricks";
			whiteBrick->Set("u_Material.AlbedoMap", ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png"));
			whiteBrick->Set("u_Material.Specular", solidGrey);
			whiteBrick->Set("u_Material.NormalMap", ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png"));
		}

		Material::Sptr normalmapMat = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			Texture2D::Sptr normalMap = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			normalmapMat->Name = "Tangent Space Normal Map";
			normalmapMat->Set("u_Material.AlbedoMap", diffuseMap);
			normalmapMat->Set("u_Material.NormalMap", normalMap);
			normalmapMat->Set("u_Material.Shininess", 0.5f);
			normalmapMat->Set("u_Scale", 0.1f);
		}

		Material::Sptr multiTextureMat = ResourceManager::CreateAsset<Material>(multiTextureShader);
		{
			Texture2D::Sptr sand = ResourceManager::CreateAsset<Texture2D>("textures/terrain/sand.png");
			Texture2D::Sptr grass = ResourceManager::CreateAsset<Texture2D>("textures/terrain/grass.png");

			multiTextureMat->Name = "Multitexturing";
			multiTextureMat->Set("u_Material.DiffuseA", sand);
			multiTextureMat->Set("u_Material.DiffuseB", grass);
			multiTextureMat->Set("u_Material.NormalMapA", normalMapDefault);
			multiTextureMat->Set("u_Material.NormalMapB", normalMapDefault);
			multiTextureMat->Set("u_Material.Shininess", 0.5f);
			multiTextureMat->Set("u_Scale", 0.1f);
		}



#pragma endregion

#pragma region Lights

		// Create some lights for our scene
		GameObject::Sptr lightParent = scene->CreateGameObject("Lights");

		for (int ix = 0; ix < 50; ix++) {
			GameObject::Sptr light = scene->CreateGameObject("Light");
			light->SetPostion(glm::vec3(glm::diskRand(25.0f), 1.0f));
			lightParent->AddChild(light);

			Light::Sptr lightComponent = light->Add<Light>();
			lightComponent->SetColor(glm::linearRand(glm::vec3(0.0f), glm::vec3(1.0f)));
			lightComponent->SetRadius(glm::linearRand(0.1f, 10.0f));
			lightComponent->SetIntensity(glm::linearRand(1.0f, 2.0f));
		}

#pragma endregion

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		MeshResource::Sptr sphere = ResourceManager::CreateAsset<MeshResource>();
		sphere->AddParam(MeshBuilderParam::CreateIcoSphere(ZERO, ONE, 5));
		sphere->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->MainCamera->GetGameObject()->SelfRef();
		{
			camera->SetPostion({ -1, -3.5f, 14.7 });
			camera->LookAt(glm::vec3(0.0f));

			//camera->Add<SimpleCameraControl>();

			// This is now handled by scene itself!
			//Camera::Sptr cam = camera->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera!
			//scene->MainCamera = cam;
		}

#pragma region Objects

		// Add some walls :3
		{
			MeshResource::Sptr wall = ResourceManager::CreateAsset<MeshResource>();
			wall->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			wall->GenerateMesh();
		
			GameObject::Sptr building1 = scene->CreateGameObject("Building 1");
			building1->Add<RenderComponent>()->SetMesh(wall)->SetMaterial(brickMaterial);
			building1->SetScale(glm::vec3(10.0f, 10.0f, 3.0f));
			building1->SetPostion(glm::vec3(5.0f, -15.0f, 1.5f));
			
			GameObject::Sptr building2 = scene->CreateGameObject("Building 2");
			building2->Add<RenderComponent>()->SetMesh(wall)->SetMaterial(brickMaterial);
			building2->SetScale(glm::vec3(10.0f, 10.0f, 5.0f));
			building2->SetPostion(glm::vec3(-5.0f, -15.0f, 1.5f));
			
			GameObject::Sptr building3 = scene->CreateGameObject("Building 3");
			building3->Add<RenderComponent>()->SetMesh(wall)->SetMaterial(brickMaterial);
			building3->SetScale(glm::vec3(10.0f, 15.0f, 3.0f));
			building3->SetPostion(glm::vec3(-5.0f, 17.0f, 1.5f));
			
			GameObject::Sptr building4 = scene->CreateGameObject("Building 4");
			building4->Add<RenderComponent>()->SetMesh(wall)->SetMaterial(brickMaterial);
			building4->SetScale(glm::vec3(10.0f, 10.0f, 5.0f));
			building4->SetPostion(glm::vec3(5.0f, 14.5f, 1.5f));
			
			GameObject::Sptr sidewalk1 = scene->CreateGameObject("Sidewalk 1");
			sidewalk1->Add<RenderComponent>()->SetMesh(wall)->SetMaterial(sidewalkMaterial);
			sidewalk1->SetScale(glm::vec3(20.0f, 3.0f, 0.25f));
			sidewalk1->SetPostion(glm::vec3(0.0f, 9.0f, 0.0f));
			
			GameObject::Sptr sidewalk2 = scene->CreateGameObject("Sidewalk 2");
			sidewalk2->Add<RenderComponent>()->SetMesh(wall)->SetMaterial(sidewalkMaterial);
			sidewalk2->SetScale(glm::vec3(20.0f, 3.0f, 0.25f));
			sidewalk2->SetPostion(glm::vec3(0.0f, -9.0f, 0.0f));
			
			GameObject::Sptr road = scene->CreateGameObject("Road");
			road->Add<RenderComponent>()->SetMesh(wall)->SetMaterial(roadMaterial);
			road->SetScale(glm::vec3(20.0f, 15.0f, 0.1f));
			road->SetPostion(glm::vec3(0.0f, 0.0f, 0.0f));			
		}

		

		GameObject::Sptr playerMonkey = scene->CreateGameObject("Player");
		{
			// Set position in the scene
			playerMonkey->SetPostion(glm::vec3(-10.0f, 0.0f, 2.0f));
			playerMonkey->SetRotation({ 0, 0, 180 });

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = playerMonkey->Add<RenderComponent>();
			renderer->SetMesh(monkeyMesh);
			renderer->SetMaterial(customMaterial);

			RigidBody::Sptr rigid = playerMonkey->Add<RigidBody>();
			rigid->SetType(RigidBodyType::Kinematic);
			BoxCollider::Sptr collider = BoxCollider::Create(glm::vec3(0.5f, 0.5f, 0.5f));
			rigid->AddCollider(collider);

			PlayerController::Sptr controller = playerMonkey->Add<PlayerController>();
			//controller->SetMatRef(renderer->GetMaterial());
		}

		GameObject::Sptr enemyMonkey = scene->CreateGameObject("Enemy");
		{
			// Set position in the scene
			enemyMonkey->SetPostion(glm::vec3(10.0f, 0.0f, 2.0f));
			enemyMonkey->SetRotation({ 0, 0, 0 });

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = enemyMonkey->Add<RenderComponent>();
			renderer->SetMesh(monkeyMesh);
			renderer->SetMaterial(customMaterial);

			EnemyController::Sptr controller = enemyMonkey->Add<EnemyController>();
			controller->SetPlayerRef(playerMonkey);
			controller->SetBulletMesh(monkeyMesh);
			controller->SetBulletMaterial(customMaterial);
		}
		

		GameObject::Sptr shadowCaster1 = scene->CreateGameObject("Shadow Light");
		{
			// Set position in the scene
			shadowCaster1->SetPostion(glm::vec3(8.85f, 13.88f, 2.67f));
			shadowCaster1->SetRotation(glm::vec3(72.32f, 36, 18));

			// Create and attach a renderer for the monkey
			ShadowCamera::Sptr shadowCam = shadowCaster1->Add<ShadowCamera>();
			shadowCam->SetProjection(glm::perspective(glm::radians(30.0f), 1.0f, 0.1f, 100.0f));
			shadowCam->SetProjectionMask(flashlight2);
			shadowCam->Intensity = 4.0f;
		}

		GameObject::Sptr shadowCaster2 = scene->CreateGameObject("Shadow Light");
		{
			// Set position in the scene
			shadowCaster2->SetPostion(glm::vec3(-9.99f, -13.88f, 2.67f));
			shadowCaster2->SetRotation(glm::vec3(72.32f, 36, -44));

			// Create and attach a renderer for the monkey
			ShadowCamera::Sptr shadowCam = shadowCaster2->Add<ShadowCamera>();
			shadowCam->SetProjection(glm::perspective(glm::radians(30.0f), 1.0f, 0.1f, 100.0f));
			shadowCam->SetProjectionMask(flashlight2);
			shadowCam->Intensity = 4.0f;
		}

		GameObject::Sptr shadowCaster3 = scene->CreateGameObject("Shadow Light");
		{
			// Set position in the scene
			shadowCaster3->SetPostion(glm::vec3(-7.45f, 11.16f, 2.67f));
			shadowCaster3->SetRotation(glm::vec3(72.32f, 36, -164));

			// Create and attach a renderer for the monkey
			ShadowCamera::Sptr shadowCam = shadowCaster3->Add<ShadowCamera>();
			shadowCam->SetProjection(glm::perspective(glm::radians(30.0f), 1.0f, 0.1f, 100.0f));
			shadowCam->SetProjectionMask(flashlight2);
			shadowCam->Intensity = 4.0f;
		}

		GameObject::Sptr shadowCaster4 = scene->CreateGameObject("Shadow Light");
		{
			// Set position in the scene
			shadowCaster4->SetPostion(glm::vec3(-9.31f, 11.16f, 2.67f));
			shadowCaster4->SetRotation(glm::vec3(76.32f, 47, 135));

			// Create and attach a renderer for the monkey
			ShadowCamera::Sptr shadowCam = shadowCaster4->Add<ShadowCamera>();
			shadowCam->SetProjection(glm::perspective(glm::radians(30.0f), 1.0f, 0.1f, 100.0f));
			shadowCam->SetProjectionMask(flashlight2);
			shadowCam->Intensity = 4.0f;			
		}

#pragma endregion

		
#pragma region UI

		/*GameObject::Sptr canvas = scene->CreateGameObject("UI Canvas"); 
		{
			RectTransform::Sptr transform = canvas->Add<RectTransform>();
			transform->SetMin({ 16, 16 });
			transform->SetMax({ 128, 128 });

			GuiPanel::Sptr canPanel = canvas->Add<GuiPanel>();


			GameObject::Sptr subPanel = scene->CreateGameObject("Sub Item");
			{
				RectTransform::Sptr transform = subPanel->Add<RectTransform>();
				transform->SetMin({ 10, 10 });
				transform->SetMax({ 64, 64 });

				GuiPanel::Sptr panel = subPanel->Add<GuiPanel>();
				panel->SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/upArrow.png"));

				Font::Sptr font = ResourceManager::CreateAsset<Font>("fonts/Roboto-Medium.ttf", 16.0f);
				font->Bake();

				GuiText::Sptr text = subPanel->Add<GuiText>();
				text->SetText("Hello world!");
				text->SetFont(font);
			}

			canvas->AddChild(subPanel);
		}*/
		
#pragma endregion

		GameObject::Sptr rain = scene->CreateGameObject("Particles"); 
		{
			rain->SetPostion({ 0.0f, 0.0f, 20.0f });

			ParticleSystem::Sptr particleManager = rain->Add<ParticleSystem>();
			particleManager->Atlas = particleTex;
			particleManager->_gravity = { 0, 0, -40 };
			particleManager->SetMaxParticles(10000);

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::BoxEmitter;
			emitter.TexID = 3;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(0.451f, 0.584f, 1.0f, 1.0f);
			emitter.Lifetime = 0.0f;
			emitter.BoxEmitterData.Timer = 1.0f / 50.0f;
			emitter.BoxEmitterData.Velocity = { 1.0f, 1.0f, 1.0f };
			emitter.BoxEmitterData.LifeRange = { 10.0f, 10.0f };
			emitter.BoxEmitterData.SizeRange = { 0.01f, 0.5f };
			emitter.BoxEmitterData.HalfExtents = { 21.9f, 19.1f, 2.5f };

			particleManager->AddEmitter(emitter);
		}

		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("scene-manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		// Send the scene to the application
		app.LoadScene(scene);
	}
}
