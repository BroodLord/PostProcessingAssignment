//--------------------------------------------------------------------------------------
// Scene geometry and layout preparation
// Scene rendering & update
//--------------------------------------------------------------------------------------

#include "Scene.h"
#include "Mesh.h"
#include "Model.h"
#include "Camera.h"
#include "State.h"
#include "Shader.h"
#include "Input.h"
#include "Common.h"

#include "CVector2.h" 
#include "CVector3.h" 
#include "CMatrix4x4.h"
#include "MathHelpers.h"     // Helper functions for maths
#include "GraphicsHelpers.h" // Helper functions to unclutter the code here
#include "ColourRGBA.h" 

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <List>
#include <array>
#include <sstream>
#include <cmath> 
#include <iomanip> 
#include <iostream>
#include <memory>


//--------------------------------------------------------------------------------------
// Scene Data
//--------------------------------------------------------------------------------------

//********************
// Available post-processes
enum class PostProcess
{
	None,
	Copy,
	Tint,
	TintHue,
	GreyNoise,
	Burn,
	Distort,
	Spiral,
	Blur,
	SecondBlur,
	Underwater,
	HeatHaze,
	NightVision,
	Pixelation,
	Scanlines,
	Inverse,
	BlackAndWhite,
	SeeingWorlds,
	SecondSeeingWorlds,
	Bloom,
	Merge,
	Sigmoid,
};

// Function to create Gaussian filter 
void FilterCreation(float GKernel[301], int& SampleAmount)
{
	// intialising standard deviation to 1.0 
	float sigma = 40;
	float r, s = 2.0 * sigma * sigma;
	int HalfSampleAmount = (((SampleAmount - 1)) / 2 + 1);
	// sum is for normalization 
	float sum = 0.0;
	// generating 5x5 kernel 
	for (int x = -HalfSampleAmount; x <= HalfSampleAmount; x++) {

		r = sqrt(x * x);
		GKernel[x + HalfSampleAmount] = (exp(-(r * r) / s)) / (3.14 * s);
		sum += GKernel[x + HalfSampleAmount];

	}

	// normalising the Kernel 
	for (int i = 0; i < SampleAmount; ++i) {

		GKernel[i] /= sum;
	}

	if (SampleAmount % 2 == 0)
	{
		SampleAmount--;
	}
}


struct PostProcessData
{
	union
	{
		struct
		{
			float rgbTop[3];
			float rgbMid[3];
			float padding; // As the GPU only allows padding of 4,8,16, not 12
			void tint(float Top[], float Mid[])
			{
				for (int i = 0; i < 3; i++)
				{
					rgbTop[i] = Top[i];
					rgbMid[i] = Mid[i];
				}
			}
		}tint;
		struct
		{
			float Hue1[3];
			float Hue2[3];
			float padding; // As the GPU only allows padding of 4,8,16, not 12
			void Hue(float Top[], float Mid[])
			{
				for (int i = 0; i < 3; i++)
				{
					Hue1[i] = Top[i];
					Hue2[i] = Mid[i];
				}
			}
		}Hue;
		struct
		{
			float grainSize;
			float padding; // As the GPU only allows padding of 4,8,16, not 12
		}Noise;
		struct
		{
			float burnSpeed;
			float padding; // As the GPU only allows padding of 4,8,16, not 12


		}Burn;
		struct
		{
			int blur;
			float padding; // As the GPU only allows padding of 4,8,16, not 12
			void Blur(int B)
			{
				blur = B;
			}

		}Blur;
		struct
		{
			float Gamma;
			float padding; // As the GPU only allows padding of 4,8,16, not 12

		}Sigmoid;
		struct
		{
			float waterSpeed;
			float padding; // As the GPU only allows padding of 4,8,16, not 12
		}Water;
		struct
		{
			float offset;
			float padding; // As the GPU only allows padding of 4,8,16, not 12
		}SeeingWorlds;
	};
};

const char* PPNames[] = {


	"None",
	"Copy",
	"Tint",
	"TintHue",
	"GreyNoise",
	"Burn",
	"Distort",
	"Spiral",
	"Blur",
	"SecondBlur",
	"Underwater",
	"HeatHaze",
	"NightVision",
	"Pixelation",
	"Scanlines",
	"Inverse",
	"BlackAndWhite",
	"SeeingWorlds",
	"SecondSeeingWorlds",
	"Bloom",
	"Merge",
	"Sigmoid",

};

const char* ModeNames[] = {
	"FullScreen",
	"Area",
	"Polygon",
	"ModelPolygon"
};


enum class PostProcessMode
{
	Fullscreen,
	Area,
	Polygon,
	ModelPolygon
};

struct ModelStruct
{
	Model* Mod;
	std::string Name;
	void Set(Model* M, std::string N)
	{
		Mod = M;
		Name = N;
	}
};

struct PostProcessingData2
{
	Model* Mod;
	std::string Name;
	int NumberOfProcessors;
	PostProcess PP;
	PostProcessMode PostProcessingMode;
	void Set(PostProcess P, PostProcessMode PM, Model* M, std::string N, int i)
	{
		Name = N;
		Mod = M;
		PP = P;
		PostProcessingMode = PM;
		NumberOfProcessors = i;

	}
};


auto gCurrentPostProcess = PostProcess::None;
auto gCurrentPostProcessMode = PostProcessMode::Fullscreen;
std::vector<PostProcessingData2> PostProcessingVector;
std::vector<ModelStruct> ModelVector;
std::vector<PostProcessData> PostProcessingDataVector;
//********************


// Constants controlling speed of movement/rotation (measured in units per second because we're using frame time)
const float ROTATION_SPEED = 1.5f;  // Radians per second for rotation
const float MOVEMENT_SPEED = 50.0f; // Units per second for movement (what a unit of length is depends on 3D model - i.e. an artist decision usually)

// Lock FPS to monitor refresh rate, which will typically set it to 60fps. Press 'p' to toggle to full fps
bool lockFPS = true;

float FrameTime;


// Meshes, models and cameras, same meaning as TL-Engine. Meshes prepared in InitGeometry function, Models & camera in InitScene
Mesh* gStarsMesh;
Mesh* gGroundMesh;
Mesh* gCubeMesh;
Mesh* gCrateMesh;
Mesh* gLightMesh;
Mesh* gWallTwoMesh;
Mesh* gWallOneMesh;

Model* gWallOne;
Model* gWallTwo;
Model* gLargeWindow;
Model* gSmallWindow1;
Model* gSmallWindow2;
Model* gSmallWindow3;
Model* gSmallWindow4;
Model* gStars;
Model* gGround;
Model* gCube;
Model* gCrate;


Camera* gCamera;


// Store lights in an array in this exercise
const int NUM_LIGHTS = 2;
struct Light
{
	Model* model;
	CVector3 colour;
	float    strength;
};
Light gLights[NUM_LIGHTS];

static int Counter = 0;
static float burnSpeed = 2.0f;
static float WaterSpeed = 1.0f;
static int Selected_Item = 0;
static int Selected_Screen = 0;

// Additional light information
CVector3 gAmbientColour = { 0.3f, 0.3f, 0.4f }; // Background level of light (slightly bluish to match the far background, which is dark blue)
float    gSpecularPower = 256; // Specular power controls shininess - same for all models in this app

ColourRGBA gBackgroundColor = { 0.3f, 0.3f, 0.4f, 1.0f };

// Variables controlling light1's orbiting of the cube
const float gLightOrbitRadius = 20.0f;
const float gLightOrbitSpeed = 0.7f;


//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
// Variables sent over to the GPU each frame
// The structures are now in Common.h
// IMPORTANT: Any new data you add in C++ code (CPU-side) is not automatically available to the GPU
//            Anything the shaders need (per-frame or per-model) needs to be sent via a constant buffer

PerFrameConstants gPerFrameConstants;      // The constants (settings) that need to be sent to the GPU each frame (see common.h for structure)
ID3D11Buffer* gPerFrameConstantBuffer; // The GPU buffer that will recieve the constants above

PerModelConstants gPerModelConstants;      // As above, but constants (settings) that change per-model (e.g. world matrix)
ID3D11Buffer* gPerModelConstantBuffer; // --"--

//**************************
PostProcessingConstants gPostProcessingConstants;       // As above, but constants (settings) for each post-process
ID3D11Buffer* gPostProcessingConstantBuffer; // --"--
//**************************


//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

// DirectX objects controlling textures used in this lab
ID3D11Resource* gStarsDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gStarsDiffuseSpecularMapSRV = nullptr;
ID3D11Resource* gGroundDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gGroundDiffuseSpecularMapSRV = nullptr;
ID3D11Resource* gCrateDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gCrateDiffuseSpecularMapSRV = nullptr;
ID3D11Resource* gCubeDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gCubeDiffuseSpecularMapSRV = nullptr;

ID3D11ShaderResourceView* gWallOneDiffuseSpecularMapSRV = nullptr;
ID3D11Resource* gWallOneDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gWallTwoDiffuseSpecularMapSRV = nullptr;
ID3D11Resource* gWallTwoDiffuseSpecularMap = nullptr;

ID3D11Resource* gLightDiffuseMap = nullptr;
ID3D11ShaderResourceView* gLightDiffuseMapSRV = nullptr;



//****************************
// Post processing textures

// This texture will have the scene renderered on it. Then the texture is then used for post-processing
ID3D11Texture2D* gSceneTexture = nullptr; // This object represents the memory used by the texture on the GPU
ID3D11RenderTargetView* gSceneRenderTarget = nullptr; // This object is used when we want to render to the texture above
ID3D11ShaderResourceView* gSceneTextureSRV = nullptr; // This object is used to give shaders access to the texture above (SRV = shader resource view)

// This texture will have the scene renderered on it. Then the texture is then used for post-processing
ID3D11Texture2D* gBackTexture = nullptr; // This object represents the memory used by the texture on the GPU
ID3D11RenderTargetView* gBackRenderTarget = nullptr; // This object is used when we want to render to the texture above
ID3D11ShaderResourceView* gBackTextureSRV = nullptr; // This object is used to give shaders access to the texture above (SRV = shader resource view)

ID3D11Texture2D* gMergeTexture = nullptr; // This object represents the memory used by the texture on the GPU
ID3D11RenderTargetView* gMergeTarget = nullptr;
ID3D11ShaderResourceView* gMergeMapSRV = nullptr;


// Additional textures used for specific post-processes

ID3D11Resource* gNoiseMap = nullptr;
ID3D11ShaderResourceView* gNoiseMapSRV = nullptr;
ID3D11Resource* gBurnMap = nullptr;
ID3D11ShaderResourceView* gBurnMapSRV = nullptr;
ID3D11Resource* gDistortMap = nullptr;
ID3D11ShaderResourceView* gDistortMapSRV = nullptr;


//****************************



//--------------------------------------------------------------------------------------
// Initialise scene geometry, constant buffers and states
//--------------------------------------------------------------------------------------

// Prepare the geometry required for the scene
// Returns true on success
bool InitGeometry()
{
	////--------------- Load meshes ---------------////

	// Load mesh geometry data, just like TL-Engine this doesn't create anything in the scene. Create a Model for that.
	try
	{
		gStarsMesh = new Mesh("Stars.x");
		gGroundMesh = new Mesh("Hills.x");
		gCubeMesh = new Mesh("Cube.x");
		gCrateMesh = new Mesh("CargoContainer.x");
		gLightMesh = new Mesh("Light.x");
		gWallOneMesh = new Mesh("Wall1.x");
		gWallTwoMesh = new Mesh("Wall2.x");
	}
	catch (std::runtime_error e)  // Constructors cannot return error messages so use exceptions to catch mesh errors (fairly standard approach this)
	{
		gLastError = e.what(); // This picks up the error message put in the exception (see Mesh.cpp)
		return false;
	}


	////--------------- Load / prepare textures & GPU states ---------------////

	// Load textures and create DirectX objects for them
	// The LoadTexture function requires you to pass a ID3D11Resource* (e.g. &gCubeDiffuseMap), which manages the GPU memory for the
	// texture and also a ID3D11ShaderResourceView* (e.g. &gCubeDiffuseMapSRV), which allows us to use the texture in shaders
	// The function will fill in these pointers with usable data. The variables used here are globals found near the top of the file.
	if (!LoadTexture("Stars.jpg", &gStarsDiffuseSpecularMap, &gStarsDiffuseSpecularMapSRV) ||
		!LoadTexture("GrassDiffuseSpecular.dds", &gGroundDiffuseSpecularMap, &gGroundDiffuseSpecularMapSRV) ||
		!LoadTexture("StoneDiffuseSpecular.dds", &gCubeDiffuseSpecularMap, &gCubeDiffuseSpecularMapSRV) ||
		!LoadTexture("CargoA.dds", &gCrateDiffuseSpecularMap, &gCrateDiffuseSpecularMapSRV) ||
		!LoadTexture("Flare.jpg", &gLightDiffuseMap, &gLightDiffuseMapSRV) ||
		!LoadTexture("Noise.png", &gNoiseMap, &gNoiseMapSRV) ||
		!LoadTexture("Burn.png", &gBurnMap, &gBurnMapSRV) ||
		!LoadTexture("Distort.png", &gDistortMap, &gDistortMapSRV) ||
		!LoadTexture("Brick_35.jpg", &gWallOneDiffuseSpecularMap, &gWallOneDiffuseSpecularMapSRV) ||
		!LoadTexture("Brick_35.jpg", &gWallTwoDiffuseSpecularMap, &gWallTwoDiffuseSpecularMapSRV))
	{
		gLastError = "Error loading textures";
		return false;
	}


	// Create all filtering modes, blending modes etc. used by the app (see State.cpp/.h)
	if (!CreateStates())
	{
		gLastError = "Error creating states";
		return false;
	}


	////--------------- Prepare shaders and constant buffers to communicate with them ---------------////

	// Load the shaders required for the geometry we will use (see Shader.cpp / .h)
	if (!LoadShaders())
	{
		gLastError = "Error loading shaders";
		return false;
	}

	// Create GPU-side constant buffers to receive the gPerFrameConstants and gPerModelConstants structures above
	// These allow us to pass data from CPU to shaders such as lighting information or matrices
	// See the comments above where these variable are declared and also the UpdateScene function
	gPerFrameConstantBuffer = CreateConstantBuffer(sizeof(gPerFrameConstants));
	gPerModelConstantBuffer = CreateConstantBuffer(sizeof(gPerModelConstants));
	gPostProcessingConstantBuffer = CreateConstantBuffer(sizeof(gPostProcessingConstants));
	if (gPerFrameConstantBuffer == nullptr || gPerModelConstantBuffer == nullptr || gPostProcessingConstantBuffer == nullptr)
	{
		gLastError = "Error creating constant buffers";
		return false;
	}



	//********************************************
	//**** Create Scene Texture

	// We will render the scene to this texture instead of the back-buffer (screen), then we post-process the texture onto the screen
	// This is exactly the same code we used in the graphics module when we were rendering the scene onto a cube using a texture

	// Using a helper function to load textures from files above. Here we create the scene texture manually
	// as we are creating a special kind of texture (one that we can render to). Many settings to prepare:
	D3D11_TEXTURE2D_DESC sceneTextureDesc = {};
	sceneTextureDesc.Width = gViewportWidth;  // Full-screen post-processing - use full screen size for texture
	sceneTextureDesc.Height = gViewportHeight;
	sceneTextureDesc.MipLevels = 1; // No mip-maps when rendering to textures (or we would have to render every level)
	sceneTextureDesc.ArraySize = 1;
	sceneTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA texture (8-bits each)
	sceneTextureDesc.SampleDesc.Count = 1;
	sceneTextureDesc.SampleDesc.Quality = 0;
	sceneTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	sceneTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; // IMPORTANT: Indicate we will use texture as render target, and pass it to shaders
	sceneTextureDesc.CPUAccessFlags = 0;
	sceneTextureDesc.MiscFlags = 0;
	if (FAILED(gD3DDevice->CreateTexture2D(&sceneTextureDesc, NULL, &gSceneTexture)))
	{
		gLastError = "Error creating scene texture";
		return false;
	}

	if (FAILED(gD3DDevice->CreateTexture2D(&sceneTextureDesc, NULL, &gBackTexture)))
	{
		gLastError = "Error creating scene texture";
		return false;
	}
	if (FAILED(gD3DDevice->CreateTexture2D(&sceneTextureDesc, NULL, &gMergeTexture)))
	{
		gLastError = "Error creating scene texture";
		return false;
	}

	// We created the scene texture above, now we get a "view" of it as a render target, i.e. get a special pointer to the texture that
	// we use when rendering to it (see RenderScene function below)
	if (FAILED(gD3DDevice->CreateRenderTargetView(gSceneTexture, NULL, &gSceneRenderTarget)))
	{
		gLastError = "Error creating scene render target view";
		return false;
	}
	if (FAILED(gD3DDevice->CreateRenderTargetView(gBackTexture, NULL, &gBackRenderTarget)))
	{
		gLastError = "Error creating scene render target view";
		return false;
	}
	if (FAILED(gD3DDevice->CreateRenderTargetView(gMergeTexture, NULL, &gMergeTarget)))
	{
		gLastError = "Error creating scene render target view";
		return false;
	}

	// We also need to send this texture (resource) to the shaders. To do that we must create a shader-resource "view"
	D3D11_SHADER_RESOURCE_VIEW_DESC srDesc = {};
	srDesc.Format = sceneTextureDesc.Format;
	srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srDesc.Texture2D.MostDetailedMip = 0;
	srDesc.Texture2D.MipLevels = 1;
	if (FAILED(gD3DDevice->CreateShaderResourceView(gSceneTexture, &srDesc, &gSceneTextureSRV)))
	{
		gLastError = "Error creating scene shader resource view";
		return false;
	}
	if (FAILED(gD3DDevice->CreateShaderResourceView(gMergeTexture, &srDesc, &gMergeMapSRV)))
	{
		gLastError = "Error creating scene shader resource view";
		return false;
	}
	if (FAILED(gD3DDevice->CreateShaderResourceView(gBackTexture, &srDesc, &gBackTextureSRV)))
	{
		gLastError = "Error creating scene shader resource view";
		return false;
	}


	return true;
}

void WindowPostProcessSetUp()
{
	PostProcessingData2 PPNM;
	PostProcessData PPD;
	gCurrentPostProcess = PostProcess::BlackAndWhite;
	PostProcessingDataVector.push_back(PPD);
	PPNM.Set(gCurrentPostProcess, PostProcessMode::ModelPolygon, gLargeWindow, "LargeWindow", 1);
	PostProcessingVector.push_back(PPNM);

	gCurrentPostProcess = PostProcess::Inverse;
	PostProcessingDataVector.push_back({});
	PPNM.Set(gCurrentPostProcess, PostProcessMode::ModelPolygon, gSmallWindow1, "SmallWindow1", 1);
	PostProcessingVector.push_back(PPNM);


	gCurrentPostProcess = PostProcess::NightVision;
	PostProcessingDataVector.push_back({});
	PPNM.Set(gCurrentPostProcess, PostProcessMode::ModelPolygon, gSmallWindow2, "SmallWindow2", 1);
	PostProcessingVector.push_back(PPNM);

	gCurrentPostProcess = PostProcess::Scanlines;
	PostProcessingDataVector.push_back({});
	PPNM.Set(gCurrentPostProcess, PostProcessMode::ModelPolygon, gSmallWindow3, "SmallWindow3", 1);
	PostProcessingVector.push_back(PPNM);

	gCurrentPostProcess = PostProcess::SeeingWorlds;
	PostProcessingDataVector.push_back({ 0.05f });
	PPNM.Set(gCurrentPostProcess, PostProcessMode::ModelPolygon, gSmallWindow4, "SmallWindow4", 2);
	PostProcessingVector.push_back(PPNM);
	gCurrentPostProcess = PostProcess::SecondSeeingWorlds;
	PostProcessingDataVector.push_back({ 0.05f });
	PPNM.Set(gCurrentPostProcess, PostProcessMode::ModelPolygon, gSmallWindow4, "SmallWindow4", 2);
	PostProcessingVector.push_back(PPNM);
}


// Prepare the scene
// Returns true on success
bool InitScene()
{
	////--------------- Set up scene ---------------////

	gStars = new Model(gStarsMesh);
	gGround = new Model(gGroundMesh);
	gCube = new Model(gCubeMesh);
	gCrate = new Model(gCrateMesh);
	gWallOne = new Model(gWallOneMesh);
	gWallTwo = new Model(gWallTwoMesh);
	gLargeWindow = new Model(gCubeMesh);
	gSmallWindow1 = new Model(gCubeMesh);
	gSmallWindow2 = new Model(gCubeMesh);
	gSmallWindow3 = new Model(gCubeMesh);
	gSmallWindow4 = new Model(gCubeMesh);


	// Initial positions
	gCube->SetPosition({ 42, 5, -10 });
	gCube->SetRotation({ 0.0f, ToRadians(-110.0f), 0.0f });
	gCube->SetScale(1.5f);
	gWallOne->SetPosition({ 30, 20, -10 });
	gWallOne->SetScale(50.0f);
	gWallTwo->SetPosition({ 30, 20, 60 });
	gWallTwo->SetScale(50.0f);
	gCrate->SetPosition({ -10, 0, 90 });
	gCrate->SetRotation({ 0.0f, ToRadians(40.0f), 0.0f });
	gCrate->SetScale(6.0f);
	gStars->SetScale(8000.0f);
	gLargeWindow->SetPosition({ 10,17,-10 });
	gSmallWindow1->SetPosition({ -12,17,60 });
	gSmallWindow2->SetPosition({ 0,17,60 });
	gSmallWindow3->SetPosition({ 17,17,60 });
	gSmallWindow4->SetPosition({ 34,17,60 });


	// Light set-up - using an array this time
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		gLights[i].model = new Model(gLightMesh);
	}

	gLights[0].colour = { 0.8f, 0.8f, 1.0f };
	gLights[0].strength = 10;
	gLights[0].model->SetPosition({ 30, 10, 0 });
	gLights[0].model->SetScale(pow(gLights[0].strength, 1.0f)); // Convert light strength into a nice value for the scale of the light - equation is ad-hoc.

	gLights[1].colour = { 1.0f, 0.8f, 0.2f };
	gLights[1].strength = 40;
	gLights[1].model->SetPosition({ -70, 30, 100 });
	gLights[1].model->SetScale(pow(gLights[1].strength, 1.0f));


	////--------------- Set up camera ---------------////

	gCamera = new Camera();
	gCamera->SetPosition({ 25, 18, -45 });
	gCamera->SetRotation({ ToRadians(10.0f), ToRadians(7.0f), 0.0f });

	ModelStruct MS;
	MS.Set(gCube, "Cube");
	ModelVector.push_back(MS);
	MS.Set(gLights[0].model, "Light_1");
	ModelVector.push_back(MS);
	MS.Set(gLights[1].model, "Light_2");
	ModelVector.push_back(MS);
	MS.Set(gLargeWindow, "LargeWindow");
	ModelVector.push_back(MS);
	MS.Set(gSmallWindow1, "SmallWindow1");
	ModelVector.push_back(MS);
	MS.Set(gSmallWindow2, "SmallWindow2");
	ModelVector.push_back(MS);
	MS.Set(gSmallWindow3, "SmallWindow3");
	ModelVector.push_back(MS);
	MS.Set(gSmallWindow4, "SmallWindow4");
	ModelVector.push_back(MS);

	WindowPostProcessSetUp();

	return true;
}


// Release the geometry and scene resources created above
void ReleaseResources()
{
	ReleaseStates();

	if (gSceneTextureSRV)              gSceneTextureSRV->Release();
	if (gSceneRenderTarget)            gSceneRenderTarget->Release();
	if (gSceneTexture)                 gSceneTexture->Release();

	if (gMergeMapSRV)                  gMergeMapSRV->Release();
	if (gMergeTarget)                  gMergeTarget->Release();
	if (gMergeTexture)                 gMergeTexture->Release();

	if (gBackTextureSRV)			   gBackTextureSRV->Release();
	if (gBackRenderTarget)			   gBackRenderTarget->Release();
	if (gBackTexture)				   gBackTexture->Release();

	if (gDistortMapSRV)                gDistortMapSRV->Release();
	if (gDistortMap)                   gDistortMap->Release();
	if (gBurnMapSRV)                   gBurnMapSRV->Release();
	if (gBurnMap)                      gBurnMap->Release();
	if (gNoiseMapSRV)                  gNoiseMapSRV->Release();
	if (gNoiseMap)                     gNoiseMap->Release();

	if (gLightDiffuseMapSRV)           gLightDiffuseMapSRV->Release();
	if (gLightDiffuseMap)              gLightDiffuseMap->Release();
	if (gCrateDiffuseSpecularMapSRV)   gCrateDiffuseSpecularMapSRV->Release();
	if (gCrateDiffuseSpecularMap)      gCrateDiffuseSpecularMap->Release();
	if (gCubeDiffuseSpecularMapSRV)    gCubeDiffuseSpecularMapSRV->Release();
	if (gCubeDiffuseSpecularMap)       gCubeDiffuseSpecularMap->Release();
	if (gGroundDiffuseSpecularMapSRV)  gGroundDiffuseSpecularMapSRV->Release();
	if (gGroundDiffuseSpecularMap)     gGroundDiffuseSpecularMap->Release();
	if (gStarsDiffuseSpecularMapSRV)   gStarsDiffuseSpecularMapSRV->Release();
	if (gStarsDiffuseSpecularMap)      gStarsDiffuseSpecularMap->Release();
	if (gWallTwoDiffuseSpecularMapSRV) gWallTwoDiffuseSpecularMapSRV->Release();
	if (gWallTwoDiffuseSpecularMap)    gWallTwoDiffuseSpecularMap->Release();
	if (gWallOneDiffuseSpecularMapSRV) gWallOneDiffuseSpecularMapSRV->Release();
	if (gWallOneDiffuseSpecularMap)    gWallOneDiffuseSpecularMap->Release();

	if (gPostProcessingConstantBuffer)  gPostProcessingConstantBuffer->Release();
	if (gPerModelConstantBuffer)        gPerModelConstantBuffer->Release();
	if (gPerFrameConstantBuffer)        gPerFrameConstantBuffer->Release();

	ReleaseShaders();

	// See note in InitGeometry about why we're not using unique_ptr and having to manually delete
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		delete gLights[i].model;  gLights[i].model = nullptr;
	}
	delete gCamera;  gCamera = nullptr;
	delete gCrate;   gCrate = nullptr;
	delete gCube;    gCube = nullptr;
	delete gGround;  gGround = nullptr;
	delete gStars;   gStars = nullptr;
	delete gWallOne; gWallOne = nullptr;
	delete gWallTwo; gWallTwo = nullptr;

	delete gWallOneMesh; gWallOneMesh = nullptr;
	delete gWallTwoMesh; gWallTwoMesh = nullptr;
	delete gLargeWindow; gLargeWindow = nullptr;
	delete gSmallWindow1; gSmallWindow1 = nullptr;
	delete gSmallWindow2; gSmallWindow2 = nullptr;
	delete gSmallWindow3; gSmallWindow3 = nullptr;
	delete gSmallWindow4; gSmallWindow4 = nullptr;
	delete gLightMesh;   gLightMesh = nullptr;
	delete gCrateMesh;   gCrateMesh = nullptr;
	delete gCubeMesh;    gCubeMesh = nullptr;
	delete gGroundMesh;  gGroundMesh = nullptr;
	delete gStarsMesh;   gStarsMesh = nullptr;
}



//--------------------------------------------------------------------------------------
// Scene Rendering
//--------------------------------------------------------------------------------------

// Render everything in the scene from the given camera
void RenderSceneFromCamera(Camera* camera)
{
	// Set camera matrices in the constant buffer and send over to GPU
	gPerFrameConstants.cameraMatrix = camera->WorldMatrix();
	gPerFrameConstants.viewMatrix = camera->ViewMatrix();
	gPerFrameConstants.projectionMatrix = camera->ProjectionMatrix();
	gPerFrameConstants.viewProjectionMatrix = camera->ViewProjectionMatrix();
	UpdateConstantBuffer(gPerFrameConstantBuffer, gPerFrameConstants);

	// Indicate that the constant buffer we just updated is for use in the vertex shader (VS), geometry shader (GS) and pixel shader (PS)
	gD3DContext->VSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer); // First parameter must match constant buffer number in the shader 
	gD3DContext->GSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);
	gD3DContext->PSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);

	gD3DContext->PSSetShader(gPixelLightingPixelShader, nullptr, 0);


	////--------------- Render ordinary models ---------------///

	// Select which shaders to use next
	gD3DContext->VSSetShader(gPixelLightingVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gPixelLightingPixelShader, nullptr, 0);
	gD3DContext->GSSetShader(nullptr, nullptr, 0);  // Switch off geometry shader when not using it (pass nullptr for first parameter)

	// States - no blending, normal depth buffer and back-face culling (standard set-up for opaque models)
	gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gUseDepthBufferState, 0);
	gD3DContext->RSSetState(gCullBackState);

	// Render lit models, only change textures for each onee
	gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);

	gD3DContext->PSSetShaderResources(0, 1, &gGroundDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gGround->Render();

	gD3DContext->PSSetShaderResources(0, 1, &gCrateDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gCrate->Render();
	gD3DContext->PSSetShaderResources(0, 1, &gWallOneDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gWallOne->Render();
	gD3DContext->PSSetShaderResources(0, 1, &gWallTwoDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gWallTwo->Render();

	gD3DContext->PSSetShaderResources(0, 1, &gCubeDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gCube->Render();


	////--------------- Render sky ---------------////

	// Select which shaders to use next
	gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gTintedTexturePixelShader, nullptr, 0);

	// Using a pixel shader that tints the texture - don't need a tint on the sky so set it to white
	gPerModelConstants.objectColour = { 1, 1, 1 };

	// Stars point inwards
	gD3DContext->RSSetState(gCullNoneState);

	// Render sky
	gD3DContext->PSSetShaderResources(0, 1, &gStarsDiffuseSpecularMapSRV);
	gStars->Render();



	////--------------- Render lights ---------------////

	// Select which shaders to use next (actually same as before, so we could skip this)
	gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gTintedTexturePixelShader, nullptr, 0);

	// Select the texture and sampler to use in the pixel shader
	gD3DContext->PSSetShaderResources(0, 1, &gLightDiffuseMapSRV); // First parameter must match texture slot number in the shaer

	// States - additive blending, read-only depth buffer and no culling (standard set-up for blending)
	gD3DContext->OMSetBlendState(gAdditiveBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
	gD3DContext->RSSetState(gCullNoneState);

	// Render all the lights in the array
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		gPerModelConstants.objectColour = gLights[i].colour; // Set any per-model constants apart from the world matrix just before calling render (light colour here)
		gLights[i].model->Render();
	}
}



//**************************

// Select the appropriate shader plus any additional textures required for a given post-process
// Helper function shared by full-screen, area and polygon post-processing functions below
void SelectPostProcessShaderAndTextures(PostProcess postProcess)
{


	// Prepare custom settings for current post-process
	if (postProcess == PostProcess::Copy)
	{
		gD3DContext->PSSetShader(gCopyPostProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::Bloom)
	{
		gD3DContext->PSSetShader(gBloomPostProcess, nullptr, 0);

	}
	else if (postProcess == PostProcess::Merge)
	{
		gD3DContext->PSSetShaderResources(1, 1, &gMergeMapSRV);
		gD3DContext->PSSetShader(gMergePostProcess, nullptr, 0);
		gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);

	}
	else if (postProcess == PostProcess::Inverse)
	{
		gD3DContext->PSSetShader(gInversePostProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::Scanlines)
	{
		gD3DContext->PSSetShader(gPredatorPostProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::NightVision)
	{
		gD3DContext->PSSetShader(gNightVisionPostProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::Blur)
	{
		float GKernel[302];
		FilterCreation(GKernel, PostProcessingDataVector[Counter].Blur.blur);
		int HalfSampleAmount = (((PostProcessingDataVector[Counter].Blur.blur - 1)) / 2 + 1);
		gPostProcessingConstants.blurStrength = PostProcessingDataVector[Counter].Blur.blur;
		for (int i = 0; i < HalfSampleAmount; i++)
		{
			gPostProcessingConstants.WeightArray[i].x = GKernel[i];
		}
		gD3DContext->PSSetShader(gBlurPostProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::SecondBlur)
	{
		gD3DContext->PSSetShader(gSecondBlurPostProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::BlackAndWhite)
	{
		gD3DContext->PSSetShader(gBlackAndWhitePostProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::SeeingWorlds)
	{
		gPostProcessingConstants.ITime += FrameTime;
		gPostProcessingConstants.OffSet = PostProcessingDataVector[Counter].SeeingWorlds.offset;
		gD3DContext->PSSetShader(gSeeingWorldsPostProcess, nullptr, 0);
		gD3DContext->PSSetShader(gSecondSeeingWorldsPostProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::Underwater)
	{
		WaterSpeed = PostProcessingDataVector[Counter].Water.waterSpeed;
		gD3DContext->PSSetShader(gUnderwaterPostProcess, nullptr, 0);

	}
	else if (postProcess == PostProcess::Pixelation)
	{
		gD3DContext->PSSetShader(gPixelationPostProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::Tint)
	{

		gPostProcessingConstants.tintColour1 = { PostProcessingDataVector[Counter].tint.rgbTop[0] ,PostProcessingDataVector[Counter].tint.rgbTop[1] ,PostProcessingDataVector[Counter].tint.rgbTop[2] };
		gPostProcessingConstants.tintColour2 = { PostProcessingDataVector[Counter].tint.rgbMid[0] ,PostProcessingDataVector[Counter].tint.rgbMid[1] ,PostProcessingDataVector[Counter].tint.rgbMid[2] };
		gD3DContext->PSSetShader(gTintPostProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::Sigmoid)
	{
		gPostProcessingConstants.Gamma = PostProcessingDataVector[Counter].Sigmoid.Gamma;
		gD3DContext->PSSetShader(gSigmoidPostProcess, nullptr, 0);
	}
	else if (postProcess == PostProcess::TintHue)
	{

		gPostProcessingConstants.tintColour1 = { PostProcessingDataVector[Counter].Hue.Hue1[0], PostProcessingDataVector[Counter].Hue.Hue1[1], PostProcessingDataVector[Counter].Hue.Hue1[2] };
		gPostProcessingConstants.tintColour2 = { PostProcessingDataVector[Counter].Hue.Hue2[0], PostProcessingDataVector[Counter].Hue.Hue2[1], PostProcessingDataVector[Counter].Hue.Hue2[2] };
		gD3DContext->PSSetShader(gTintHuePostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::GreyNoise)
	{
		gD3DContext->PSSetShader(gGreyNoisePostProcess, nullptr, 0);
		float grainSize; // Fineness of the noise grain
		grainSize = PostProcessingDataVector[Counter].Noise.grainSize;
		gPostProcessingConstants.noiseScale = { gViewportWidth / grainSize, gViewportHeight / grainSize };
		// Give pixel shader access to the noise texture
		gD3DContext->PSSetShaderResources(1, 1, &gNoiseMapSRV);
		gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);
	}

	else if (postProcess == PostProcess::Burn)
	{

		gD3DContext->PSSetShader(gBurnPostProcess, nullptr, 0);

		burnSpeed = PostProcessingDataVector[Counter].Burn.burnSpeed;
		// Give pixel shader access to the burn texture (basically a height map that the burn level ascends)
		gD3DContext->PSSetShaderResources(1, 1, &gBurnMapSRV);
		gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);
	}

	else if (postProcess == PostProcess::Distort)
	{
		gD3DContext->PSSetShader(gDistortPostProcess, nullptr, 0);

		// Give pixel shader access to the distortion texture (containts 2D vectors (in R & G) to shift the texture UVs to give a cut-glass impression)
		gD3DContext->PSSetShaderResources(1, 1, &gDistortMapSRV);
		gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);
	}

	else if (postProcess == PostProcess::Spiral)
	{
		gD3DContext->PSSetShader(gSpiralPostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::HeatHaze)
	{
		gD3DContext->PSSetShader(gHeatHazePostProcess, nullptr, 0);
	}

}



// Perform a full-screen post process from "scene texture" to back buffer
void FullScreenPostProcess(PostProcess postProcess)
{



	// Using special vertex shader that creates its own data for a 2D screen quad
	gD3DContext->VSSetShader(g2DQuadVertexShader, nullptr, 0);
	gD3DContext->GSSetShader(nullptr, nullptr, 0);  // Switch off geometry shader when not using it (pass nullptr for first parameter)


	// States - no blending, don't write to depth buffer and ignore back-face culling
	gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
	gD3DContext->RSSetState(gCullNoneState);


	// No need to set vertex/index buffer (see 2D quad vertex shader), just indicate that the quad will be created as a triangle strip
	gD3DContext->IASetInputLayout(NULL); // No vertex data
	gD3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ID3D11ShaderResourceView* nullSRV = nullptr;
	gD3DContext->PSSetShaderResources(0, 1, &nullSRV);
	if (Counter % 2 == 0)
	{
		// Select the back buffer to use for rendering. Not going to clear the back-buffer because we're going to overwrite it all
		gD3DContext->OMSetRenderTargets(1, &gBackRenderTarget, gDepthStencil);


		// Give the pixel shader (post-processing shader) access to the scene texture 
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV/* MISSING select the scene texture shader resource view (note: needs an &)*/);
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)

	}
	else
	{
		// Select the back buffer to use for rendering. Not going to clear the back-buffer because we're going to overwrite it all
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget, gDepthStencil);

		// Give the pixel shader (post-processing shader) access to the scene texture 
		gD3DContext->PSSetShaderResources(0, 1, &gBackTextureSRV/* MISSING select the scene texture shader resource view (note: needs an &)*/);
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)
	}

	// Select shader and textures needed for the required post-processes (helper function above)
	SelectPostProcessShaderAndTextures(postProcess);


	// Set 2D area for full-screen post-processing (coordinates in 0->1 range)
	gPostProcessingConstants.area2DTopLeft = { 0, 0 }; // Top-left of entire screen
	gPostProcessingConstants.area2DSize = { 1, 1 }; // Full size of screen
	gPostProcessingConstants.area2DDepth = 0;        // Depth buffer value for full screen is as close as possible


	// Pass over the above post-processing settings (also the per-process settings prepared in UpdateScene function below)
	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
	gD3DContext->VSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
	gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);


	// Draw a quad
	gD3DContext->Draw(4, 0);

	gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);

	gD3DContext->Draw(4, 0);





}


// Perform an area post process from "scene texture" to back buffer at a given point in the world, with a given size (world units)
void AreaPostProcess(PostProcess postProcess, CVector3 worldPoint, CVector2 areaSize, float ZShift)
{
	//// First perform a full-screen copy of the scene to back-buffer
	FullScreenPostProcess(PostProcess::Copy);
	//
	//// Now perform a post-process of a portion of the scene to the back-buffer (overwriting some of the copy above)
	//// Note: The following code relies on many of the settings that were prepared in the FullScreenPostProcess call above, it only
	////       updates a few things that need to be changed for an area process. If you tinker with the code structure you need to be
	////       aware of all the work that the above function did that was also preparation for this post-process area step
	//
	//// Select shader/textures needed for required post-process

	ID3D11ShaderResourceView* nullSRV = nullptr;
	gD3DContext->PSSetShaderResources(0, 1, &nullSRV);
	if (Counter % 2 == 0)
	{
		// Select the back buffer to use for rendering. Not going to clear the back-buffer because we're going to overwrite it all
		gD3DContext->OMSetRenderTargets(1, &gBackRenderTarget, gDepthStencil);


		// Give the pixel shader (post-processing shader) access to the scene texture 
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV/* MISSING select the scene texture shader resource view (note: needs an &)*/);
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)

	}
	else
	{
		// Select the back buffer to use for rendering. Not going to clear the back-buffer because we're going to overwrite it all
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget, gDepthStencil);

		// Give the pixel shader (post-processing shader) access to the scene texture 
		gD3DContext->PSSetShaderResources(0, 1, &gBackTextureSRV/* MISSING select the scene texture shader resource view (note: needs an &)*/);
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)
	}

	SelectPostProcessShaderAndTextures(postProcess);

	// Enable alpha blending - area effects need to fade out at the edges or the hard edge of the area is visible
	// A couple of the shaders have been updated to put the effect into a soft circle
	// Alpha blending isn't enabled for fullscreen and polygon effects so it doesn't affect those (except heat-haze, which works a bit differently)



	gD3DContext->OMSetBlendState(gAlphaBlendingState, nullptr, 0xffffff);


	// Use picking methods to find the 2D position of the 3D point at the centre of the area effect
	auto worldPointTo2D = gCamera->PixelFromWorldPt(worldPoint, gViewportWidth, gViewportHeight);
	CVector2 area2DCentre = { worldPointTo2D.x, worldPointTo2D.y };
	float areaDistance = worldPointTo2D.z / ZShift;

	// Nothing to do if given 3D point is behind the camera
	if (areaDistance < gCamera->NearClip())  return;

	// Convert pixel coordinates to 0->1 coordinates as used by the shader
	area2DCentre.x /= gViewportWidth;
	area2DCentre.y /= gViewportHeight;



	// Using new helper function here - it calculates the world space units covered by a pixel at a certain distance from the camera.
	// Use this to find the size of the 2D area we need to cover the world space size requested
	CVector2 pixelSizeAtPoint = gCamera->PixelSizeInWorldSpace(areaDistance, gViewportWidth, gViewportHeight);
	CVector2 area2DSize = { areaSize.x / pixelSizeAtPoint.x, areaSize.y / pixelSizeAtPoint.y };

	// Again convert the result in pixels to a result to 0->1 coordinates
	area2DSize.x /= gViewportWidth;
	area2DSize.y /= gViewportHeight;



	// Send the area top-left and size into the constant buffer - the 2DQuad vertex shader will use this to create a quad in the right place
	gPostProcessingConstants.area2DTopLeft = area2DCentre - 0.5f * area2DSize; // Top-left of area is centre - half the size
	gPostProcessingConstants.area2DSize = area2DSize;

	// Manually calculate depth buffer value from Z distance to the 3D point and camera near/far clip values. Result is 0->1 depth value
	// We've never seen this full calculation before, it's occasionally useful. It is derived from the material in the Picking lecture
	// Having the depth allows us to have area effects behind normal objects
	gPostProcessingConstants.area2DDepth = gCamera->FarClip() * (areaDistance - gCamera->NearClip()) / (gCamera->FarClip() - gCamera->NearClip());
	gPostProcessingConstants.area2DDepth /= areaDistance;

	// Pass over this post-processing area to shaders (also sends the per-process settings prepared in UpdateScene function below)
	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
	gD3DContext->VSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
	gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);

	// Draw a quad
	gD3DContext->Draw(4, 0);

	gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);

	gD3DContext->Draw(4, 0);

}


// Perform an post process from "scene texture" to back buffer within the given four-point polygon and a world matrix to position/rotate/scale the polygon
void PolygonPostProcess(PostProcess postProcess, const std::array<CVector3, 4>& points, const CMatrix4x4& worldMatrix)
{
	// First perform a full-screen copy of the scene to back-buffer
	FullScreenPostProcess(PostProcess::Copy);


	// Now perform a post-process of a portion of the scene to the back-buffer (overwriting some of the copy above)
	// Note: The following code relies on many of the settings that were prepared in the FullScreenPostProcess call above, it only
	//       updates a few things that need to be changed for an area process. If you tinker with the code structure you need to be
	//       aware of all the work that the above function did that was also preparation for this post-process area step

	// Select shader/textures needed for required post-process

	ID3D11ShaderResourceView* nullSRV = nullptr;
	gD3DContext->PSSetShaderResources(0, 1, &nullSRV);
	if (Counter % 2 == 0)
	{
		// Select the back buffer to use for rendering. Not going to clear the back-buffer because we're going to overwrite it all
		gD3DContext->OMSetRenderTargets(1, &gBackRenderTarget, gDepthStencil);


		// Give the pixel shader (post-processing shader) access to the scene texture 
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV/* MISSING select the scene texture shader resource view (note: needs an &)*/);
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)

	}
	else
	{
		// Select the back buffer to use for rendering. Not going to clear the back-buffer because we're going to overwrite it all
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget, gDepthStencil);

		// Give the pixel shader (post-processing shader) access to the scene texture 
		gD3DContext->PSSetShaderResources(0, 1, &gBackTextureSRV/* MISSING select the scene texture shader resource view (note: needs an &)*/);
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)
	}

	SelectPostProcessShaderAndTextures(postProcess);

	// Loop through the given points, transform each to 2D (this is what the vertex shader normally does in most labs)
	for (unsigned int i = 0; i < points.size(); ++i)
	{
		CVector4 modelPosition = CVector4(points[i], 1);
		CVector4 worldPosition = modelPosition * worldMatrix;
		CVector4 viewportPosition = worldPosition * gCamera->ViewProjectionMatrix();

		gPostProcessingConstants.polygon2DPoints[i] = viewportPosition;
	}

	// Pass over the polygon points to the shaders (also sends the per-process settings prepared in UpdateScene function below)
	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
	gD3DContext->VSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
	gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);

	// Select the special 2D polygon post-processing vertex shader and draw the polygon
	gD3DContext->VSSetShader(g2DPolygonVertexShader, nullptr, 0);
	gD3DContext->Draw(4, 0);
	gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);

	gD3DContext->Draw(4, 0);

}




// Rendering the scene
void RenderScene()
{
	//IMGUI
	//*******************************
	// Prepare ImGUI for this frame
	//*******************************

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//*******************************

	//// Common settings ////

	// Set up the light information in the constant buffer
	// Don't send to the GPU yet, the function RenderSceneFromCamera will do that
	gPerFrameConstants.light1Colour = gLights[0].colour * gLights[0].strength;
	gPerFrameConstants.light1Position = gLights[0].model->Position();
	gPerFrameConstants.light2Colour = gLights[1].colour * gLights[1].strength;
	gPerFrameConstants.light2Position = gLights[1].model->Position();

	gPerFrameConstants.ambientColour = gAmbientColour;
	gPerFrameConstants.specularPower = gSpecularPower;
	gPerFrameConstants.cameraPosition = gCamera->Position();

	gPerFrameConstants.viewportWidth = static_cast<float>(gViewportWidth);
	gPerFrameConstants.viewportHeight = static_cast<float>(gViewportHeight);

	if (PostProcessingVector.size() != 0)
	{
		gD3DContext->OMSetRenderTargets(1, &gMergeTarget, gDepthStencil);
		gD3DContext->ClearRenderTargetView(gMergeTarget, &gBackgroundColor.r);
		gD3DContext->ClearDepthStencilView(gDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	// Setup the viewport to the size of the main window
	D3D11_VIEWPORT vp;
	vp.Width = static_cast<FLOAT>(gViewportWidth);
	vp.Height = static_cast<FLOAT>(gViewportHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	gD3DContext->RSSetViewports(1, &vp);


	// Render the scene from the main camera
	RenderSceneFromCamera(gCamera);
	

	////--------------- Main scene rendering ---------------////

	// Set the target for rendering and select the main depth buffer.
	// If using post-processing then render to the scene texture, otherwise to the usual back buffer
	// Also clear the render target to a fixed colour and the depth buffer to the far distance

	if (PostProcessingVector.size() != 0)
	{
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget, gDepthStencil);
		gD3DContext->ClearRenderTargetView(gSceneRenderTarget, &gBackgroundColor.r);
	}
	else
	{
		gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);
		gD3DContext->ClearRenderTargetView(gBackBufferRenderTarget, &gBackgroundColor.r);
	}
	gD3DContext->ClearDepthStencilView(gDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Setup the viewport to the size of the main window
	//D3D11_VIEWPORT vp;
	vp.Width = static_cast<FLOAT>(gViewportWidth);
	vp.Height = static_cast<FLOAT>(gViewportHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	gD3DContext->RSSetViewports(1, &vp);

	// Render the scene from the main camera
	RenderSceneFromCamera(gCamera);


	////--------------- Scene completion ---------------////

	// Run any post-processing steps
	if (PostProcessingVector.size() != 0)
	{
		for (int i = 0; i < PostProcessingVector.size(); i++)
		{
			if (PostProcessingVector[i].Mod == gLargeWindow || PostProcessingVector[i].Mod == gSmallWindow1 || PostProcessingVector[i].Mod == gSmallWindow2 ||
				PostProcessingVector[i].Mod == gSmallWindow3 || PostProcessingVector[i].Mod == gSmallWindow4)
			{
				if (PostProcessingVector[i].PostProcessingMode != PostProcessMode::Fullscreen)
				{
					PostProcessingVector[i].PostProcessingMode = PostProcessMode::ModelPolygon;
				}
			}
			if (PostProcessingVector[i].PostProcessingMode == PostProcessMode::Fullscreen)
			{
				FullScreenPostProcess(PostProcessingVector[i].PP);
				Counter++;
			}


			if (PostProcessingVector[i].PostProcessingMode == PostProcessMode::Area)
			{
				// Pass a 3D point for the centre of the affected area and the size of the (rectangular) area in world units
				AreaPostProcess(PostProcessingVector[i].PP, PostProcessingVector[i].Mod->Position(), { 10, 10 }, 3.2f);
				Counter++;
			}

			else if (PostProcessingVector[i].PostProcessingMode == PostProcessMode::Polygon)
			{
				// An array of four points in world space - a tapered square centred at the origin
				const std::array<CVector3, 4> points = { { {-5, 5,0}, {-5,-5,0}, {5,5,0},{5,-5,0} } }; // C++ strangely needs an extra pair of {} here... only for std:array...

				// A rotating matrix placing the model above in the scene
				static CMatrix4x4 polyMatrix = MatrixTranslation({ 20, 15, 0 });
				polyMatrix = MatrixRotationY(ToRadians(1)) * polyMatrix;

				// Pass an array of 4 points and a matrix. Only supports 4 points.
				PolygonPostProcess(PostProcessingVector[i].PP, points, polyMatrix);
				Counter++;


			}

			else if (PostProcessingVector[i].PostProcessingMode == PostProcessMode::ModelPolygon)
			{
				// An array of four points in world space - a tapered square centred at the origin
				const std::array<CVector3, 4> points = { { {PostProcessingVector[i].Mod->Position().x - 7, PostProcessingVector[i].Mod->Position().y + 7,PostProcessingVector[i].Mod->Position().z},
														   {PostProcessingVector[i].Mod->Position().x - 7,PostProcessingVector[i].Mod->Position().y - 7,PostProcessingVector[i].Mod->Position().z},
														   {PostProcessingVector[i].Mod->Position().x + 7,PostProcessingVector[i].Mod->Position().y + 7,PostProcessingVector[i].Mod->Position().z},
														   {PostProcessingVector[i].Mod->Position().x + 7, PostProcessingVector[i].Mod->Position().y - 7,PostProcessingVector[i].Mod->Position().z} } }; // C++ strangely needs an extra pair of {} here... only for std:array...

				// A rotating matrix placing the model above in the scene
				static CMatrix4x4 polyMatrix = MatrixTranslation({ 20, 15, 0 });
				//polyMatrix = MatrixRotationY(ToRadians(1)) * polyMatrix;

				// Pass an array of 4 points and a matrix. Only supports 4 points.
				PolygonPostProcess(PostProcessingVector[i].PP, points, polyMatrix);
				Counter++;


			}
		}
		Counter = 0;



		// These lines unbind the scene texture from the pixel shader to stop DirectX issuing a warning when we try to render to it again next frame
		ID3D11ShaderResourceView* nullSRV = nullptr;
		gD3DContext->PSSetShaderResources(0, 1, &nullSRV);
	}

	//IMGUI
	//*******************************
	// Draw ImGUI interface
	//*******************************
	// You can draw ImGUI elements at any time between the frame preparation code at the top
	// of this function, and the finalisation code below

	ImGui::Begin("Model & Screen_Mode Selection", 0, ImGuiWindowFlags_AlwaysAutoResize);
	int i = 0;
	ImGui::Text("Useable models for *Area* PostProcessers");
	for (std::vector<ModelStruct>::iterator it = ModelVector.begin(); it != ModelVector.end() - 5; ++it) {
		std::string HashTag = "##" + std::to_string(i);
		if (ImGui::Selectable(HashTag.c_str(), i == Selected_Item))
		{
			Selected_Item = i;
			Selected_Screen = 1;
			gCurrentPostProcessMode = PostProcessMode::Area;
		}

		ImGui::SameLine();
		ImGui::Text("Model: ");
		ImGui::SameLine();
		ImGui::Text((*it).Name.c_str());
		i++;
	}
	ImGui::Text("Useable models for *ModelPolygon* PostProcessers");
	for (std::vector<ModelStruct>::iterator it = ModelVector.begin() + 3; it != ModelVector.end(); ++it) {
		std::string HashTag = "##" + std::to_string(i);
		if (ImGui::Selectable(HashTag.c_str(), i == Selected_Item))
		{
			Selected_Item = i;
			Selected_Screen = 3;
			gCurrentPostProcessMode = PostProcessMode::ModelPolygon;
		}

		ImGui::SameLine();
		ImGui::Text("Model: ");
		ImGui::SameLine();
		ImGui::Text((*it).Name.c_str());
		i++;
	}
	if (ImGui::Button("FullScreen", ImVec2(130, 20)))
	{
		Selected_Screen = 0;
		gCurrentPostProcessMode = PostProcessMode::Fullscreen;
	}
	ImGui::Text("Current Screen Mode: ");
	ImGui::SameLine();
	ImGui::Text(ModeNames[Selected_Screen]);
	ImGui::End();

	ImGui::Begin("PostProcessingWindow", 0, ImGuiWindowFlags_AlwaysAutoResize);
	if (ImGui::BeginMenu("Add A Post Process"))
	{
		if (ImGui::Button("Tint", ImVec2(100, 20))) {
			gCurrentPostProcess = PostProcess::Tint;
			PostProcessData PPD;
			float tempTop[3] = { 0.3f,0.8f,0.0f };
			float tempMid[3] = { 0.1f,0.5f,1.0f };
			PPD.tint.tint(tempTop, tempMid);
			PostProcessingDataVector.push_back(PPD);
			PostProcessingData2 PPNM;
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 1);
			PostProcessingVector.push_back(PPNM);
		}
		if (ImGui::Button("TintHue", ImVec2(100, 20)))
		{
			gCurrentPostProcess = PostProcess::TintHue;
			PostProcessData PPD;
			float tempHue1[3] = { 0.3f,0.8f,0.0f };
			float tempHue2[3] = { 0.1f,0.5f,1.0f };
			PPD.Hue.Hue(tempHue1, tempHue2);
			PostProcessingDataVector.push_back({ PPD });
			PostProcessingData2 PPNM;
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 1);
			PostProcessingVector.push_back(PPNM);
		}
		if (ImGui::Button("Blur", ImVec2(100, 20)))
		{

			PostProcessData Blur;
			PostProcessingData2 PPNM;
			Blur.Blur.Blur(5);
			gCurrentPostProcess = PostProcess::Blur;
			PostProcessingDataVector.push_back(Blur);
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 2);
			PostProcessingVector.push_back(PPNM);
			gCurrentPostProcess = PostProcess::SecondBlur;
			PostProcessingDataVector.push_back({});
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 2);
			PostProcessingVector.push_back(PPNM);
		}
		if (ImGui::Button("Sigmoid", ImVec2(100, 20)))
		{
			gCurrentPostProcess = PostProcess::Sigmoid;
			PostProcessingDataVector.push_back({ 0.25 });
			PostProcessingData2 PPNM;
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 1);
			PostProcessingVector.push_back(PPNM);
		}
		if (ImGui::Button("Bloom", ImVec2(100, 20)))
		{
			PostProcessingData2 PPNM;
			PostProcessData PPD;
			gCurrentPostProcess = PostProcess::Bloom;
			PostProcessingDataVector.push_back({});
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 4);
			PostProcessingVector.push_back(PPNM);
			gCurrentPostProcess = PostProcess::Blur;
			PostProcessData Blur;
			Blur.Blur.Blur(30);
			PostProcessingDataVector.push_back(Blur);
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 4);
			PostProcessingVector.push_back(PPNM);
			gCurrentPostProcess = PostProcess::SecondBlur;
			PostProcessingDataVector.push_back({});
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 4);
			PostProcessingVector.push_back(PPNM);
			gCurrentPostProcess = PostProcess::Merge;
			PostProcessingDataVector.push_back({});
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 4);
			PostProcessingVector.push_back(PPNM);
		}
		if (ImGui::Button("Burn", ImVec2(100, 20)))
		{
			gCurrentPostProcess = PostProcess::Burn;
			PostProcessingDataVector.push_back({ 1.0f });
			PostProcessingData2 PPNM;
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 1);
			PostProcessingVector.push_back(PPNM);
		}
		if (ImGui::Button("Inverse", ImVec2(100, 20)))
		{
			gCurrentPostProcess = PostProcess::Inverse;
			PostProcessingDataVector.push_back({});
			PostProcessingData2 PPNM;
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 1);
			PostProcessingVector.push_back(PPNM);
		}
		if (ImGui::Button("Distort", ImVec2(100, 20)))
		{
			gCurrentPostProcess = PostProcess::Distort;
			PostProcessingDataVector.push_back({});
			PostProcessingData2 PPNM;
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 1);
			PostProcessingVector.push_back(PPNM);
		}
		if (ImGui::Button("Spiral", ImVec2(100, 20)))
		{
			gCurrentPostProcess = PostProcess::Spiral;
			PostProcessingDataVector.push_back({});
			PostProcessingData2 PPNM;
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 1);
			PostProcessingVector.push_back(PPNM);
		}
		if (ImGui::Button("HeatHaze", ImVec2(100, 20)))
		{
			gCurrentPostProcess = PostProcess::HeatHaze;
			PostProcessingDataVector.push_back({});
			PostProcessingData2 PPNM;
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 1);
			PostProcessingVector.push_back(PPNM);
		}
		if (ImGui::Button("GreyNoise", ImVec2(100, 20)))
		{
			gCurrentPostProcess = PostProcess::GreyNoise;
			PostProcessingDataVector.push_back({ 140.0f });
			PostProcessingData2 PPNM;
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 1);
			PostProcessingVector.push_back(PPNM);

		}
		if (ImGui::Button("SeeingWorlds", ImVec2(100, 20)))
		{
			gCurrentPostProcess = PostProcess::SeeingWorlds;
			PostProcessingDataVector.push_back({ 0.05 });
			PostProcessingData2 PPNM;
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 2);
			PostProcessingVector.push_back(PPNM);
			gCurrentPostProcess = PostProcess::SecondSeeingWorlds;
			PostProcessingDataVector.push_back({ 0.05 });
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 2);
			PostProcessingVector.push_back(PPNM);
		}
		if (ImGui::Button("Underwater", ImVec2(100, 20)))
		{
			gCurrentPostProcess = PostProcess::Underwater;
			PostProcessingDataVector.push_back({ 1.0f });
			PostProcessingData2 PPNM;
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 1);
			PostProcessingVector.push_back(PPNM);
		}
		if (ImGui::Button("NightVision", ImVec2(100, 20))) {
			gCurrentPostProcess = PostProcess::NightVision;
			PostProcessingDataVector.push_back({});
			PostProcessingData2 PPNM;
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 1);
			PostProcessingVector.push_back(PPNM);
		}
		if (ImGui::Button("Pixelation", ImVec2(100, 20))) {
			gCurrentPostProcess = PostProcess::Pixelation;
			PostProcessingDataVector.push_back({});
			PostProcessingData2 PPNM;
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 1);
			PostProcessingVector.push_back(PPNM);
		}
		if (ImGui::Button("Scanlines", ImVec2(100, 20))) {
			gCurrentPostProcess = PostProcess::Scanlines;
			PostProcessingDataVector.push_back({});
			PostProcessingData2 PPNM;
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 1);
			PostProcessingVector.push_back(PPNM);
		}
		if (ImGui::Button("Black&White", ImVec2(100, 20))) {
			gCurrentPostProcess = PostProcess::BlackAndWhite;
			PostProcessingDataVector.push_back({});
			PostProcessingData2 PPNM;
			PPNM.Set(gCurrentPostProcess, gCurrentPostProcessMode, ModelVector[Selected_Item].Mod, ModelVector[Selected_Item].Name, 1);
			PostProcessingVector.push_back(PPNM);
		}
		ImGui::SameLine();
		ImGui::EndMenu();
	}
	if (ImGui::Button("Clear Screen", ImVec2(100, 20)))
	{
		gCurrentPostProcess = PostProcess::None;
		PostProcessingVector.clear();
		PostProcessingDataVector.clear();
	}

	bool Gloom = false;
	ImGui::Text("Active Postprocessers");
	for (int i = 0; i < PostProcessingVector.size(); i++)
	{

		ImGui::PushID(i);
		if (PPNames[(int)PostProcessingVector[i].PP] != "SecondBlur") //)
		{
			if (PPNames[(int)PostProcessingVector[i].PP] != "SecondSeeingWorlds")
			{
				if (PPNames[(int)PostProcessingVector[i].PP] != "Merge")
				{
					if (!Gloom)
					{
						if (ImGui::Button("Up", ImVec2(20, 20)))
						{

							if (PPNames[(int)PostProcessingVector[i].PP] == "Blur" || PPNames[(int)PostProcessingVector[i].PP] == "SeeingWorlds")
							{
								if (i - 1 >= 0)
								{
									if (PostProcessingVector[i - 1].NumberOfProcessors == 1)
									{
										std::swap(PostProcessingVector[i], PostProcessingVector[i - 1]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i - 1]);
										std::swap(PostProcessingVector[i + 1], PostProcessingVector[i]);
										std::swap(PostProcessingDataVector[i + 1], PostProcessingDataVector[i]);
									}
									else if (PostProcessingVector[i - 1].NumberOfProcessors == 2)
									{
										std::swap(PostProcessingVector[i], PostProcessingVector[i - 2]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i - 2]);
										std::swap(PostProcessingVector[i + 1], PostProcessingVector[i - 1]);
										std::swap(PostProcessingDataVector[i + 1], PostProcessingDataVector[i - 1]);
									}
									else if (PostProcessingVector[i - 1].NumberOfProcessors == 4)
									{
										std::swap(PostProcessingVector[i - 4], PostProcessingVector[i]);
										std::swap(PostProcessingDataVector[i - 4], PostProcessingDataVector[i]);
										std::swap(PostProcessingVector[i - 3], PostProcessingVector[i + 1]);
										std::swap(PostProcessingDataVector[i - 3], PostProcessingDataVector[i + 1]);
										std::swap(PostProcessingVector[i - 2], PostProcessingVector[i]);
										std::swap(PostProcessingDataVector[i - 2], PostProcessingDataVector[i]);
										std::swap(PostProcessingVector[i - 1], PostProcessingVector[i + 1]);
										std::swap(PostProcessingDataVector[i - 1], PostProcessingDataVector[i + 1]);
									}
								}
							}
							else if (PPNames[(int)PostProcessingVector[i].PP] == "Bloom")
							{
								if (i - 1 >= 0)
								{
									if (PostProcessingVector[i - 1].NumberOfProcessors == 1)
									{

										std::swap(PostProcessingVector[i], PostProcessingVector[i - 1]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i - 1]);
										std::swap(PostProcessingVector[i + 1], PostProcessingVector[i]);
										std::swap(PostProcessingDataVector[i + 1], PostProcessingDataVector[i]);
										std::swap(PostProcessingVector[i + 2], PostProcessingVector[i + 1]);
										std::swap(PostProcessingDataVector[i + 2], PostProcessingDataVector[i + 1]);
										std::swap(PostProcessingVector[i + 3], PostProcessingVector[i + 2]);
										std::swap(PostProcessingDataVector[i + 3], PostProcessingDataVector[i + 2]);
									}
									else if (PostProcessingVector[i - 1].NumberOfProcessors == 2)
									{
										std::swap(PostProcessingVector[i], PostProcessingVector[i - 2]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i - 2]);
										std::swap(PostProcessingVector[i + 1], PostProcessingVector[i - 1]);
										std::swap(PostProcessingDataVector[i + 1], PostProcessingDataVector[i - 1]);
										std::swap(PostProcessingVector[i + 2], PostProcessingVector[i]);
										std::swap(PostProcessingDataVector[i + 2], PostProcessingDataVector[i]);
										std::swap(PostProcessingVector[i + 3], PostProcessingVector[i + 1]);
										std::swap(PostProcessingDataVector[i + 3], PostProcessingDataVector[i + 1]);
									}
									else if (PostProcessingVector[i - 1].NumberOfProcessors == 4)
									{
										std::swap(PostProcessingVector[i], PostProcessingVector[i - 4]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i - 4]);
										std::swap(PostProcessingVector[i + 1], PostProcessingVector[i - 3]);
										std::swap(PostProcessingDataVector[i + 1], PostProcessingDataVector[i - 3]);
										std::swap(PostProcessingVector[i + 2], PostProcessingVector[i - 2]);
										std::swap(PostProcessingDataVector[i + 2], PostProcessingDataVector[i - 2]);
										std::swap(PostProcessingVector[i + 3], PostProcessingVector[i - 1]);
										std::swap(PostProcessingDataVector[i + 3], PostProcessingDataVector[i - 1]);
									}
								}
							}
							else
							{
								if (i - 1 >= 0)
								{
									if (PostProcessingVector[i - 1].NumberOfProcessors == 1)
									{
										std::swap(PostProcessingVector[i], PostProcessingVector[i - 1]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i - 1]);
									}
									else if (PostProcessingVector[i - 1].NumberOfProcessors == 2)
									{
										std::swap(PostProcessingVector[i], PostProcessingVector[i - 2]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i - 2]);
										std::swap(PostProcessingVector[i], PostProcessingVector[i - 1]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i - 1]);
									}
									else if (PostProcessingVector[i - 1].NumberOfProcessors == 4)
									{
										std::swap(PostProcessingVector[i], PostProcessingVector[i - 4]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i - 4]);
										std::swap(PostProcessingVector[i], PostProcessingVector[i - 3]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i - 3]);
										std::swap(PostProcessingVector[i], PostProcessingVector[i - 2]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i - 2]);
										std::swap(PostProcessingVector[i], PostProcessingVector[i - 1]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i - 1]);
									}
								}
							}
						}
						ImGui::SameLine();
						if (ImGui::Button("Down", ImVec2(35, 20)))
						{

							if (PPNames[(int)PostProcessingVector[i].PP] == "Blur" || PPNames[(int)PostProcessingVector[i].PP] == "SeeingWorlds")
							{
								if (i + PostProcessingVector[i].NumberOfProcessors < PostProcessingVector.size())
								{
									if (PostProcessingVector[i + 2].NumberOfProcessors == 1)
									{
										std::swap(PostProcessingVector[i], PostProcessingVector[i + 1]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i + 1]);
										std::swap(PostProcessingVector[i], PostProcessingVector[i + 2]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i + 2]);
									}
									else if (PostProcessingVector[i + 2].NumberOfProcessors == 2)
									{
										std::swap(PostProcessingVector[i], PostProcessingVector[i + 2]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i + 2]);
										std::swap(PostProcessingVector[i + 1], PostProcessingVector[i + 3]);
										std::swap(PostProcessingDataVector[i + 1], PostProcessingDataVector[i + 3]);
									}
									else if (PostProcessingVector[i + 2].NumberOfProcessors == 4)
									{
										std::swap(PostProcessingVector[i], PostProcessingVector[i + 4]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i + 4]);
										std::swap(PostProcessingVector[i + 1], PostProcessingVector[i + 5]);
										std::swap(PostProcessingDataVector[i + 1], PostProcessingDataVector[i + 3]);
										std::swap(PostProcessingVector[i + 3], PostProcessingVector[i + 1]);
										std::swap(PostProcessingDataVector[i + 3], PostProcessingDataVector[i + 1]);
										std::swap(PostProcessingVector[i + 2], PostProcessingVector[i]);
										std::swap(PostProcessingDataVector[i + 2], PostProcessingDataVector[i]);
									}
								}
							}
							else if (PPNames[(int)PostProcessingVector[i].PP] == "Bloom")
							{
								if (i + PostProcessingVector[i].NumberOfProcessors < PostProcessingVector.size())
								{
									if (PostProcessingVector[i + 4].NumberOfProcessors == 1)
									{

										std::swap(PostProcessingVector[i], PostProcessingVector[i + 4]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i + 4]);
										std::swap(PostProcessingVector[i + 1], PostProcessingVector[i + 4]);
										std::swap(PostProcessingDataVector[i + 1], PostProcessingDataVector[i + 4]);
										std::swap(PostProcessingVector[i + 2], PostProcessingVector[i + 4]);
										std::swap(PostProcessingDataVector[i + 2], PostProcessingDataVector[i + 4]);
										std::swap(PostProcessingVector[i + 3], PostProcessingVector[i + 4]);
										std::swap(PostProcessingDataVector[i + 3], PostProcessingDataVector[i + 4]);
									}
									else if (PostProcessingVector[i + 4].NumberOfProcessors == 2)
									{
										std::swap(PostProcessingVector[i], PostProcessingVector[i + 2]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i + 2]);
										std::swap(PostProcessingVector[i + 1], PostProcessingVector[i + 3]);
										std::swap(PostProcessingDataVector[i + 1], PostProcessingDataVector[i + 3]);
										std::swap(PostProcessingVector[i], PostProcessingVector[i + 4]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i + 4]);
										std::swap(PostProcessingVector[i + 1], PostProcessingVector[i + 5]);
										std::swap(PostProcessingDataVector[i + 1], PostProcessingDataVector[i + 5]);
									}
									else if (PostProcessingVector[i + 4].NumberOfProcessors == 4)
									{
										std::swap(PostProcessingVector[i], PostProcessingVector[i + 4]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i + 4]);
										std::swap(PostProcessingVector[i + 1], PostProcessingVector[i + 5]);
										std::swap(PostProcessingDataVector[i + 1], PostProcessingDataVector[i + 5]);
										std::swap(PostProcessingVector[i + 2], PostProcessingVector[i + 6]);
										std::swap(PostProcessingDataVector[i + 2], PostProcessingDataVector[i + 6]);
										std::swap(PostProcessingVector[i + 3], PostProcessingVector[i + 7]);
										std::swap(PostProcessingDataVector[i + 3], PostProcessingDataVector[i + 7]);
									}
								}
							}
							else
							{
								if (i + PostProcessingVector[i].NumberOfProcessors < PostProcessingVector.size())
								{
									if (PostProcessingVector[i + 1].NumberOfProcessors == 1)
									{
										std::swap(PostProcessingVector[i], PostProcessingVector[i + 1]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i + 1]);
									}
									else if (PostProcessingVector[i + 1].NumberOfProcessors == 2)
									{
										std::swap(PostProcessingVector[i], PostProcessingVector[i + 2]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i + 2]);
										std::swap(PostProcessingVector[i], PostProcessingVector[i + 1]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i + 1]);
									}
									else if (PostProcessingVector[i + 1].NumberOfProcessors == 4)
									{
										std::swap(PostProcessingVector[i], PostProcessingVector[i + 4]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i + 4]);
										std::swap(PostProcessingVector[i], PostProcessingVector[i + 3]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i + 3]);
										std::swap(PostProcessingVector[i], PostProcessingVector[i + 2]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i + 2]);
										std::swap(PostProcessingVector[i], PostProcessingVector[i + 1]);
										std::swap(PostProcessingDataVector[i], PostProcessingDataVector[i + 1]);
									}
								}
							}
						}
						ImGui::SameLine();
						if (ImGui::Button("x", ImVec2(15, 20)))
						{

							if (PPNames[(int)PostProcessingVector[i].PP] == "Blur" || PPNames[(int)PostProcessingVector[i].PP] == "SeeingWorlds")
							{
								PostProcessingVector.erase(PostProcessingVector.begin() + i);
								PostProcessingDataVector.erase(PostProcessingDataVector.begin() + i);
								PostProcessingVector.erase(PostProcessingVector.begin() + (i));
								PostProcessingDataVector.erase(PostProcessingDataVector.begin() + (i));
								ImGui::PopID();
							}
							else if (PPNames[(int)PostProcessingVector[i].PP] == "Bloom")
							{
								PostProcessingVector.erase(PostProcessingVector.begin() + i);
								PostProcessingDataVector.erase(PostProcessingDataVector.begin() + i);
								PostProcessingVector.erase(PostProcessingVector.begin() + (i));
								PostProcessingDataVector.erase(PostProcessingDataVector.begin() + (i));
								PostProcessingVector.erase(PostProcessingVector.begin() + i);
								PostProcessingDataVector.erase(PostProcessingDataVector.begin() + i);
								PostProcessingVector.erase(PostProcessingVector.begin() + (i));
								PostProcessingDataVector.erase(PostProcessingDataVector.begin() + (i));

								ImGui::PopID();
							}
							else
							{
								PostProcessingVector.erase(PostProcessingVector.begin() + i);
								PostProcessingDataVector.erase(PostProcessingDataVector.begin() + i);
								ImGui::PopID();

							}
							break;
						}
						ImGui::SameLine();


					}
					else
					{
						Gloom = false;
					}
				}

			}
		}

		ImGui::Text(PPNames[(int)PostProcessingVector[i].PP]);
		ImGui::SameLine();
		ImGui::Text("(");
		ImGui::SameLine();
		ImGui::Text(ModeNames[(int)PostProcessingVector[i].PostProcessingMode]);
		ImGui::SameLine();
		if (ModeNames[(int)PostProcessingVector[i].PostProcessingMode] == "FullScreen")
		{
			ImGui::Text("");

		}
		else
		{
			ImGui::Text(PostProcessingVector[i].Name.c_str());
		}
		ImGui::SameLine();
		ImGui::Text(")");
		if (PPNames[(int)PostProcessingVector[i].PP] == "Tint")
		{
			ImGui::SameLine();
			if (ImGui::BeginMenu("ColourPicker"))
			{
				float tintG0[3] = { 0.0f, 0.0f, 0.0f };
				ImGui::ColorEdit3("Tint Editor - Gradiant 1", PostProcessingDataVector[i].tint.rgbTop, ImGuiColorEditFlags_PickerHueWheel);
				float tintG1[3] = { 0.0f, 0.0f, 0.0f };
				ImGui::ColorEdit3("Tint Editor - Gradiant 2", PostProcessingDataVector[i].tint.rgbMid, ImGuiColorEditFlags_PickerHueWheel);
				ImGui::EndMenu();
			}
		}
		if (PPNames[(int)PostProcessingVector[i].PP] == "Underwater")
		{
			ImGui::SameLine();
			if (ImGui::BeginMenu("Water Properties"))
			{
				ImGui::SliderFloat("WaterSpeed", &PostProcessingDataVector[i].Noise.grainSize, 0.0f, 2.0f);
				ImGui::EndMenu();
			}
		}
		if (PPNames[(int)PostProcessingVector[i].PP] == "SeeingWorlds")
		{
			ImGui::SameLine();
			if (ImGui::BeginMenu("SeeingWorlds Properties"))
			{
				ImGui::SliderFloat("Offset", &PostProcessingDataVector[i].SeeingWorlds.offset, 0.01f, 0.09f);
				ImGui::EndMenu();
			}
		}
		if (PPNames[(int)PostProcessingVector[i].PP] == "Blur")
		{
			ImGui::SameLine();
			if (ImGui::BeginMenu("Blur Properties"))
			{

				ImGui::SliderInt("BlurStrength", &PostProcessingDataVector[i].Blur.blur, 1, 151);
				ImGui::EndMenu();
			}
		}
		if (PPNames[(int)PostProcessingVector[i].PP] == "Sigmoid")
		{
			ImGui::SameLine();
			if (ImGui::BeginMenu("Sigmoid Properties"))
			{

				ImGui::SliderFloat("Gamma", &PostProcessingDataVector[i].Sigmoid.Gamma, 0.01, 0.4);
				ImGui::EndMenu();
			}
		}
		//if (PPNames[(int)PostProcessingVector[i].PP] == "SecondBlur")
		//{
		//	ImGui::SameLine();
		//	if (ImGui::BeginMenu("SecondBlur Properties"))
		//	{
		//		ImGui::SliderInt("BlurStrength", &PostProcessingDataVector[i].Blur.blur, 0, 20);
		//		ImGui::EndMenu();
		//	}
		//}
		if (PPNames[(int)PostProcessingVector[i].PP] == "GreyNoise")
		{
			ImGui::SameLine();
			if (ImGui::BeginMenu("Noise Properties"))
			{
				ImGui::SliderFloat("GrainSize", &PostProcessingDataVector[i].Noise.grainSize, 0.0f, 380.0f);
				ImGui::EndMenu();
			}
		}
		if (PPNames[(int)PostProcessingVector[i].PP] == "Burn")
		{
			ImGui::SameLine();
			if (ImGui::BeginMenu("Burn Properties"))
			{
				ImGui::SliderFloat("BurnSpeed", &PostProcessingDataVector[i].Burn.burnSpeed, 0.0f, 2.0f);
				ImGui::EndMenu();
			}
		}
		if (PPNames[(int)PostProcessingVector[i].PP] == "Bloom")
		{
			Gloom = true;
		}
		ImGui::PopID();

	}
	ImGui::End();


	//*******************************



	////--------------- Scene completion ---------------////

	//IMGUI
	//*******************************
	// Finalise ImGUI for this frame
	//*******************************
	ImGui::Render();
	gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, nullptr);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// When drawing to the off-screen back buffer is complete, we "present" the image to the front buffer (the screen)
	// Set first parameter to 1 to lock to vsync
	gSwapChain->Present(lockFPS ? 1 : 0, 0);
}


//--------------------------------------------------------------------------------------
// Scene Update
//--------------------------------------------------------------------------------------


// Update models and camera. frameTime is the time passed since the last frame
void UpdateScene(float frameTime)
{
	FrameTime = frameTime;

	// Post processing settings - all data for post-processes is updated every frame whether in use or not (minimal cost)

	// Colour for tint shader

	// Noise scaling adjusts how fine the grey noise is.

	// The noise offset is randomised to give a constantly changing noise effect (like tv static)
	gPostProcessingConstants.noiseOffset = { Random(0.0f, 1.0f),Random(0.0f, 1.0f) };

	// Set and increase the burn level (cycling back to 0 when it reaches 1.0f)
	gPostProcessingConstants.burnHeight = fmod(gPostProcessingConstants.burnHeight + (burnSpeed * FrameTime), 1.0f);
	static float HueSpeed = 0.5f;

	gPostProcessingConstants.HueLevel += frameTime;

	gPostProcessingConstants.WaterLevel += WaterSpeed * frameTime;



	// Set the level of distortion
	gPostProcessingConstants.distortLevel = 0.03f;

	static float wiggle = 0.0f;
	static float wiggleSpeed = 1.0f;
	// Set and increase the amount of spiral - use a tweaked cos wave to animate
	gPostProcessingConstants.spiralLevel = ((1.0f - cos(wiggle)) * 4.0f);
	wiggle += wiggleSpeed * frameTime;

	// Update heat haze timer
	gPostProcessingConstants.heatHazeTimer += frameTime;

	//***********


	// Orbit one light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float lightRotate = 0.0f;
	static bool go = true;
	gLights[0].model->SetPosition({ 20 + cos(lightRotate) * gLightOrbitRadius, 10, 20 + sin(lightRotate) * gLightOrbitRadius });
	if (go)  lightRotate -= gLightOrbitSpeed * frameTime;
	if (KeyHit(Key_L))  go = !go;

	// Control of camera
	gCamera->Control(frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D);

	// Toggle FPS limiting
	if (KeyHit(Key_P))  lockFPS = !lockFPS;

	// Show frame time / FPS in the window title //
	const float fpsUpdateTime = 0.5f; // How long between updates (in seconds)
	static float totalFrameTime = 0;
	static int frameCount = 0;
	totalFrameTime += frameTime;
	++frameCount;
	if (totalFrameTime > fpsUpdateTime)
	{
		// Displays FPS rounded to nearest int, and frame time (more useful for developers) in milliseconds to 2 decimal places
		float avgFrameTime = totalFrameTime / frameCount;
		std::ostringstream frameTimeMs;
		frameTimeMs.precision(2);
		frameTimeMs << std::fixed << avgFrameTime * 1000;
		std::string windowTitle = "CO3303 Week 14: Area Post Processing - Frame Time: " + frameTimeMs.str() +
			"ms, FPS: " + std::to_string(static_cast<int>(1 / avgFrameTime + 0.5f));
		SetWindowTextA(gHWnd, windowTitle.c_str());
		totalFrameTime = 0;
		frameCount = 0;
	}
}
