///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declare the global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear the allocated memory
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// destroy the created OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformation()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		// find the defined material that matches the tag
		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			// pass the material properties into the shader
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;

	// loading .jpg textures for shapes
	bReturn = CreateGLTexture("../../Utilities/textures/wood.jpg", "wood");
	bReturn = CreateGLTexture("../../Utilities/textures/plant.jpg", "plant");
	bReturn = CreateGLTexture("../../Utilities/textures/black_marble.jpg", "marble");
	bReturn = CreateGLTexture("../../Utilities/textures/tile.jpg", "tile");
	bReturn = CreateGLTexture("../../Utilities/textures/coffee.png", "coffee");
	bReturn = CreateGLTexture("../../Utilities/textures/metallic.jpg", "metallic");
	bReturn = CreateGLTexture("../../Utilities/textures/silver_floral.jpeg", "silver");
	bReturn = CreateGLTexture("../../Utilities/textures/gold.jpg", "gold");
	bReturn = CreateGLTexture("../../Utilities/textures/gold2.jpeg", "gold2");
	bReturn = CreateGLTexture("../../Utilities/textures/pavers.jpg", "floor");
	bReturn = CreateGLTexture("../../Utilities/textures/gold-seamless-texture.jpg", "cylinder");
	bReturn = CreateGLTexture("../../Utilities/textures/circular-brushed-gold-texture.jpg", "cylinder_top");
	bReturn = CreateGLTexture("../../Utilities/textures/rusticwood.jpg", "plank");
	bReturn = CreateGLTexture("../../Utilities/textures/tilesf2.jpg", "box");
	bReturn = CreateGLTexture("../../Utilities/textures/stainedglass.jpg", "ball");
	bReturn = CreateGLTexture("../../Utilities/textures/abstract.jpg", "cone");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/
	/*********************************************************** *
	DefineObjectMaterials() * *
	This method is used for configuring the various material *
	settings for all of the objects in the 3D scene.
	***********************************************************/

	// Define the material properties for gold
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(1.0f, 0.9f, 0.6f); // Warm gold tone
	goldMaterial.ambientStrength = 0.5f; // Enhanced ambient reflection
	goldMaterial.diffuseColor = glm::vec3(0.8f, 0.6f, 0.2f); // Rich gold base color
	goldMaterial.specularColor = glm::vec3(1.0f, 0.8f, 0.6f); // Bright highlights
	goldMaterial.shininess = 2.0f; // Higher shininess for metallic effect
	goldMaterial.tag = "gold";
	m_objectMaterials.push_back(goldMaterial);

	// Define the material properties for cement
	OBJECT_MATERIAL cementMaterial;
	cementMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f); // Neutral gray tone
	cementMaterial.ambientStrength = 0.2f; // Low ambient reflection
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f); // Base cement color
	cementMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f); // Reduced reflectivity
	cementMaterial.shininess = 2.0f; // Matte appearance
	cementMaterial.tag = "cement";
	m_objectMaterials.push_back(cementMaterial);

	// Define the material properties for wood
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.5f, 0.3f, 0.1f); // Rich brown ambient tone
	woodMaterial.ambientStrength = 0.3f; // Moderate ambient reflection
	woodMaterial.diffuseColor = glm::vec3(0.6f, 0.4f, 0.2f); // Natural wood color
	woodMaterial.specularColor = glm::vec3(0.2f, 0.15f, 0.1f); // Subtle sheen
	woodMaterial.shininess = 8.0f; // Slightly polished but not overly shiny
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	// Define the material properties for tile
	OBJECT_MATERIAL tileMaterial;
	tileMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.4f); // Cool, muted tone
	tileMaterial.ambientStrength = 0.4f; // Moderate ambient reflection
	tileMaterial.diffuseColor = glm::vec3(0.5f, 0.4f, 0.3f); // Earthy tile color
	tileMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f); // Strong reflectivity
	tileMaterial.shininess = 24.0f; // Glossy polished finish
	tileMaterial.tag = "tile";
	m_objectMaterials.push_back(tileMaterial);

	// Define the material properties for glass
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.3f, 0.4f, 0.4f); // Light blue tint
	glassMaterial.ambientStrength = 0.1f; // Low ambient reflection
	glassMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f); // Nearly transparent
	glassMaterial.specularColor = glm::vec3(1.8f, 1.8f, 1.8f); // Very bright highlights
	glassMaterial.shininess = 64.0f; // High gloss and reflection
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	// Define the material properties for clay
	OBJECT_MATERIAL clayMaterial;
	clayMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.2f); // Warm, earthy tone
	clayMaterial.ambientStrength = 0.3f; // Moderate ambient reflection
	clayMaterial.diffuseColor = glm::vec3(0.6f, 0.5f, 0.4f); // Natural clay color
	clayMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f); // Matte, minimal reflections
	clayMaterial.shininess = 4.0f; // Slightly rough surface
	clayMaterial.tag = "clay";
	m_objectMaterials.push_back(clayMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable custom lighting
	m_pShaderManager->setBoolValue("bUseLighting", true);

	// Key Light (Weaker sunlight simulation)
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 0.1f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.1f);

	// Fill Light (Darker shadow softener)
	m_pShaderManager->setVec3Value("lightSources[1].position", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 0.1f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.1f);

	// Rim Light (Minimal edge highlights)
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 0.1f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.1f);

	// Background Light (Subtle ambiance)
	m_pShaderManager->setVec3Value("lightSources[3].position", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 0.1f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.1f);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	glm::mat4 scale;
	glm::mat4 rotation;
	glm::mat4 rotation2;
	glm::mat4 translation;
	glm::mat4 model;

	// CREATING DESKTOP

	// BOOK #1 (PAGES)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.6f, 0.2f, 3.0f); // wide , tall , deep

	XrotationDegrees = 3.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.5f, 4.5f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // Pure White

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// BOOK #1 (BOX)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.6f, 0.3f, 3.0f); // wide , tall , deep

	XrotationDegrees = 3.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.5f, 4.7f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1.0, 0.85, 0.0, 1.0); // Bright Yellow

	SetShaderTexture("plant");
	SetShaderMaterial("cement");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// BOOK #2 (PAGES)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.6f, 0.3f, 3.0f); // wide , tall , deep

	XrotationDegrees = 3.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.5f, 4.0f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // Pure White

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// BOOK #2 (BOX)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.6f, 0.3f, 3.0f); // wide , tall , deep

	XrotationDegrees = 3.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.5f, 4.3f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 1.0, 0.0, 1.0); // GREEN
	SetShaderMaterial("cement");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// BOOK #3 (PAGES)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.8f, 0.4f, 3.0f); // wide , tall , deep

	XrotationDegrees = 3.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.5f, 3.5f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // Pure White


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// BOOK #3 (BOX)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.8f, 0.2f, 3.0f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.5f, 3.75f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("abstract");
	SetShaderMaterial("cement");
	SetTextureUVScale(1.0, 1.0);
	SetShaderColor(1.0, 0.5, 0.0, 1.0); // ORANGE

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/

	// PENCIL #1 - CONE (Dark Tip)
	scaleXYZ = glm::vec3(0.1f, 0.1f, 0.2f); // Wide, tall, deep
	XrotationDegrees = 0.0f;  // No tilt
	YrotationDegrees = 0.0f;  // No spin
	ZrotationDegrees = 0.0f;  // No lean
	positionXYZ = glm::vec3(6.0f, 5.0f, -1.0f); // Right, downward, forward (close together)
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.3, 0.15, 0.05, 1.0); // DARK BROWN
	m_basicMeshes->DrawConeMesh(); // Draw cone mesh

	// PENCIL #1 - CONE (Tip)
	scaleXYZ = glm::vec3(0.1f, 0.2f, 0.2f); // Wide, tall, deep
	XrotationDegrees = 0.0f;  // No tilt
	YrotationDegrees = 0.0f;  // No spin
	ZrotationDegrees = 0.0f;  // No lean
	positionXYZ = glm::vec3(6.0f, 5.0f, -1.0f); // Right, downward, forward (close together)
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.6, 0.4, 0.2, 1.0); // Darker wood color
	m_basicMeshes->DrawConeMesh(); // Draw cone mesh

	// PENCIL #1 - CYLINDER (Body)
	scaleXYZ = glm::vec3(0.1f, 0.8f, 0.2f); // Wide, tall, deep
	XrotationDegrees = 0.0f;  // No tilt
	YrotationDegrees = 0.0f;  // No spin
	ZrotationDegrees = 0.0f;  // No lean
	positionXYZ = glm::vec3(6.0f, 5.0f, -1.0f); // Right, downward, forward (close together)
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0, 0.85, 0.0, 1.0); // Bright yellow
	SetShaderMaterial("clay");
	m_basicMeshes->DrawConeMesh(); // Draw cylinder mesh

	// PENCIL #2 - CONE (Dark Tip)
	scaleXYZ = glm::vec3(0.1f, 0.1f, 0.2f); // Wide, tall, deep
	XrotationDegrees = 0.0f;  // No tilt
	YrotationDegrees = 0.0f;  // No spin
	ZrotationDegrees = 0.0f;  // No lean
	positionXYZ = glm::vec3(5.7f, 5.0f, -1.0f); // Right, downward, forward (close together)
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.3, 0.15, 0.05, 1.0); // DARK BROWN
	m_basicMeshes->DrawConeMesh(); // Draw cone mesh

	// PENCIL #2 - CONE (Tip)
	scaleXYZ = glm::vec3(0.1f, 0.2f, 0.2f); // Wide, tall, deep
	XrotationDegrees = 0.0f;  // No tilt
	YrotationDegrees = 0.0f;  // No spin
	ZrotationDegrees = 0.0f;  // No lean
	positionXYZ = glm::vec3(5.7f, 5.0f, -1.0f); // Right, downward, forward (close together)
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.6, 0.4, 0.2, 1.0); // Darker wood color
	m_basicMeshes->DrawConeMesh(); // Draw cone mesh

	// PENCIL #2 - CYLINDER (Body)
	scaleXYZ = glm::vec3(0.1f, 0.8f, 0.2f); // Wide, tall, deep
	XrotationDegrees = 0.0f;  // No tilt
	YrotationDegrees = 0.0f;  // No spin
	ZrotationDegrees = 0.0f;  // No lean
	positionXYZ = glm::vec3(5.7f, 5.0f, -1.0f); // Right, downward, forward (close together)
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0, 0.85, 0.0, 1.0); // Bright yellow
	SetShaderMaterial("clay");
	m_basicMeshes->DrawConeMesh(); // Draw cylinder mesh

	/******************************************************************/

	// PENCIL HOLDER (inside)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.1f, 1.0f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.8f, 5.0f, -1.3f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1.0); // VERY DARK GRAY
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// PENCIL HOLDER
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 2.0f, 1.0f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.8f, 3.0f, -1.3f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.0, 0.5, 1.0); // PURPLE

	SetShaderTexture("tile");
	SetShaderMaterial("cement");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/******************************************************************/

	// TORUS - (inside rim)
	// set the XYZ scale for the mesh

	scaleXYZ = glm::vec3(1.0f, 0.1f, 1.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.5f, 5.6f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 0.25, 0.1, 1.0); // BROWN
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// TORUS - (cup rim)
	// set the XYZ scale for the mesh

	scaleXYZ = glm::vec3(1.2f, 0.2f, 1.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.5f, 5.5f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.3, 0.0, 0.3, 1.0); // DARK PURPLE

	SetShaderTexture("marble");
	SetShaderMaterial("gold");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// CYLINDER - (coffee mug)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 2.5f, 1.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.5f, 3.0f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.0, 0.5, 1.0); // PURPLE

	SetShaderTexture("gold");
	SetShaderMaterial("cement");
	//SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// TORUS - (coffee cup handle)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 1.5f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.0f, 4.5f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.0, 0.5, 1.0); // PURPLE

	SetShaderTexture("gold");
	SetShaderMaterial("cement");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/******************************************************************/

	// iMAC (white screen)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.5f, 10.0f, 4.0f); // wide , tall , deep

	XrotationDegrees = 75.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.3f, -2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (silver screen)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 10.0f, 5.0f); // wide , tall , deep

	XrotationDegrees = 75.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.5f, -3.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.7, 0.7, 0.7, 1.0); // Light Gray

	SetShaderTexture("metallic");
	SetShaderMaterial("cement");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/******************************************************************/

	// iMAC (mouse scroll ball)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.2f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 3.55f, 1.7f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 0.0, 0.0, 1.0); // RED
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	
	// iMAC (iMac mouse)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.1f, 1.0f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 3.55f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.7, 0.7, 0.7, 1.0); // LIGHT GREY

	SetShaderTexture("metallic");
	SetShaderMaterial("glass");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/

	// KEYBOARD ROW #1

	/******************************************************************/

	// iMAC (key letter #1)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.4f, 3.75f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #1)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.4f, 3.55f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 3.75f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 3.55f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #3)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.6f, 3.75f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #3)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.6f, 3.55f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #4)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.2f, 3.75f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #4)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.2f, 3.55f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #5)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.8f, 3.75f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #5)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.8f, 3.55f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #6)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.4f, 3.75f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #6)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.4f, 3.55f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #7)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.75f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #7)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.55f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #8)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.4f, 3.75f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #8)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.4f, 3.55f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #9)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.8f, 3.75f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #9)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.8f, 3.55f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #10)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.2f, 3.75f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #10)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.2f, 3.55f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/

	// KEYBOARD ROW #2

	/******************************************************************/

	// iMAC (key letter #1)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.4f, 3.75f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #1)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.4f, 3.55f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 3.75f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 3.55f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #3)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.6f, 3.75f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #3)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.6f, 3.55f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #4)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.2f, 3.75f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #4)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.2f, 3.55f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #5)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.8f, 3.75f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #5)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.8f, 3.55f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #6)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.4f, 3.75f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #6)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.4f, 3.55f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #7)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.75f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #7)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.55f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #8)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.4f, 3.75f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #8)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.4f, 3.55f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #9)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.8f, 3.75f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #9)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.8f, 3.55f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #10)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.2f, 3.75f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #10)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.2f, 3.55f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/

	// KEYBOARD ROW #3

	/******************************************************************/

	// iMAC (key letter #1)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.4f, 3.75f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #1)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.4f, 3.55f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 3.75f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 3.55f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #3)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.6f, 3.75f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #3)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.6f, 3.55f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #4)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.2f, 3.75f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #4)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.2f, 3.55f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #5)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.8f, 3.75f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #5)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.8f, 3.55f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #6)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.4f, 3.75f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #6)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.4f, 3.55f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #7)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.75f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #7)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.55f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #8)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.4f, 3.75f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #8)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.4f, 3.55f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #9)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.8f, 3.75f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #9)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.8f, 3.55f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #10)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.2f, 3.75f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #10)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.2f, 3.55f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (keyboard spacebar)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.8f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.60f, 3.55f, 2.9f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (keyboard spacebar)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.4f, 3.55f, 2.9f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/

	// KEYBOARD ROW #1 NUMBERS

	/******************************************************************/

	// iMAC (key number #1)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 3.75f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard number #1)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 3.55f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key number #2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.4f, 3.75f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard number #2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.4f, 3.55f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key number #3)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.8f, 3.75f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard number #3)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.8f, 3.55f, 1.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #1)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.8f, 3.75f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/******************************************************************/

	// KEYBOARD ROW #2 NUMBERS

	/******************************************************************/

	// iMAC (key letter #1)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 3.75f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #1)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 3.55f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.4f, 3.75f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.4f, 3.55f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #3)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.8f, 3.75f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #3)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.8f, 3.55f, 2.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/

	// KEYBOARD ROW #3 NUMBERS

	/******************************************************************/

	// iMAC (key letter #1)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 3.75f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #1)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 3.55f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.4f, 3.75f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.4f, 3.55f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (key letter #3)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.03f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.8f, 3.75f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0, 0.0, 0.0, 1.0); // BLACK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// iMAC (keyboard key #3)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.3f, 0.2f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.8f, 3.55f, 2.5f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0); // WHITE
	SetShaderTexture("metallic");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// iMAC (iMac keyboard)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 0.1f, 1.0f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.1f, 3.55f, 2.2f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.7, 0.7, 0.7, 1.0); // LIGHT GREY

	SetShaderTexture("metallic");
	SetShaderMaterial("cement");
	//SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();



	/******************************************************************/

	// iMAC (iMac base)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 10.0f, 3.0f); // wide , tall , deep

	XrotationDegrees = 90.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.5f, 3.0f, -1.8f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 1.0); // GREY

	SetShaderTexture("metallic");
	SetShaderMaterial("cement");
	//SetTextureUVScale(2.0, 2.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/******************************************************************/

	// table top
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(22.0f, 1.0f, 10.0f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.0f, 0.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1.0, 0.5, 0.0, 1.0); // ORANGE

	SetShaderTexture("marble");
	SetShaderMaterial("gold");
	//SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// table top (bottom)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(22.0f, 5.0f, 10.0f); // wide , tall , deep

	XrotationDegrees = 0.0f; // x = tilt forward/backward
	YrotationDegrees = 0.0f; // y = spin left/right
	ZrotationDegrees = 0.0f; // z = lean left/right

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f); // right , downward , forward

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.2, 0.2, 0.2, 1.0); // DARK GREY
	
	SetShaderTexture("plank");
	SetShaderMaterial("clay");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/

	/*
	// BACKGROUND
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	*/

	/****************************************************************/
}
