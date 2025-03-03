///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////


#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

// declaration of global variables
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
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	//initialize the texture collection
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
	// free the allocated objects
	m_pShaderManager = NULL;
	if (NULL != m_basicMeshes)
	{
		delete m_basicMeshes;
		m_basicMeshes = NULL;
	}

	// free the allocated OpenGL textures
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
 *  SetTransformations()
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

	modelView = translation * rotationZ * rotationY * rotationX * scale;

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

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	goldMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.6f);
	goldMaterial.shininess = 52.0;
	goldMaterial.tag = "metal";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	woodMaterial.shininess = 0.1;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.shininess = 95.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL plateMaterial;
	plateMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	plateMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	plateMaterial.shininess = 30.0;
	plateMaterial.tag = "plate";

	m_objectMaterials.push_back(plateMaterial);

	OBJECT_MATERIAL cheeseMaterial;
	cheeseMaterial.diffuseColor = glm::vec3(0.6f, 0.5f, 0.3f);
	cheeseMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	cheeseMaterial.shininess = 0.1;
	cheeseMaterial.tag = "cheese";

	m_objectMaterials.push_back(cheeseMaterial);

	OBJECT_MATERIAL breadMaterial;
	breadMaterial.diffuseColor = glm::vec3(0.7f, 0.6f, 0.5f);
	breadMaterial.specularColor = glm::vec3(0.02f, 0.02f, 0.02f);
	breadMaterial.shininess = 0.001;
	breadMaterial.tag = "bread";

	m_objectMaterials.push_back(breadMaterial);

	OBJECT_MATERIAL darkBreadMaterial;
	darkBreadMaterial.diffuseColor = glm::vec3(0.5f, 0.4f, 0.3f);
	darkBreadMaterial.specularColor = glm::vec3(0.01f, 0.01f, 0.01f);
	darkBreadMaterial.shininess = 0.001;
	darkBreadMaterial.tag = "darkbread";

	m_objectMaterials.push_back(darkBreadMaterial);

	OBJECT_MATERIAL shiplapMaterial;
	shiplapMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.9f);
	shiplapMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	shiplapMaterial.shininess = 2.0;
	shiplapMaterial.tag = "shiplap";

	m_objectMaterials.push_back(shiplapMaterial);

	OBJECT_MATERIAL grapeMaterial;
	grapeMaterial.diffuseColor = glm::vec3(0.4f, 0.2f, 0.4f);
	grapeMaterial.specularColor = glm::vec3(0.1f, 0.05f, 0.1f);
	grapeMaterial.shininess = 0.55;
	grapeMaterial.tag = "grape";

	m_objectMaterials.push_back(grapeMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

	// Enable lighting in the shader
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Directional light setup
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.05f, -0.3f, -0.1f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", .18f, .18f, .18f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Point light 1
	m_pShaderManager->setVec3Value("pointLights[0].position", -15.0f, 17.0f, 5.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.7f, 0.7f, 0.7f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Point light 2
	m_pShaderManager->setVec3Value("pointLights[1].position", 15.0f, 17.0f, 5.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// Point light 3
	m_pShaderManager->setVec3Value("pointLights[2].position", -15.0f, 17.0f, 6.0f);
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[2].specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setFloatValue("pointLights[2].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[2].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[2].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);

	// Point light 4
	m_pShaderManager->setVec3Value("pointLights[3].position", 15.0f, 17.0f, 6.0f);
	m_pShaderManager->setVec3Value("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[3].diffuse", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[3].specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setFloatValue("pointLights[3].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[3].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[3].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", true);

	/*// Point light 5
	m_pShaderManager->setVec3Value("pointLights[4].position", -3.2f, 6.0f, -4.0f);
	m_pShaderManager->setVec3Value("pointLights[4].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[4].diffuse", 0.9f, 0.9f, 0.9f);
	m_pShaderManager->setVec3Value("pointLights[4].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("pointLights[4].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[4].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[4].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[4].bActive", true);

	 //Spotlight setup
	m_pShaderManager->setVec3Value("spotLight.ambient", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("spotLight.specular", 0.7f, 0.7f, 0.7f);
	m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
	m_pShaderManager->setFloatValue("spotLight.linear", 0.09f);
	m_pShaderManager->setFloatValue("spotLight.quadratic", 0.032f);
	m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(42.5f)));
	m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(48.0f)));
	m_pShaderManager->setBoolValue("spotLight.bActive", true);*/
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
	bool bReturn = false;

	
	bReturn = CreateGLTexture(
		"textures/dark_wood_floor.JPG",
		"floor");

	bReturn = CreateGLTexture(
		"textures/shiplap.JPG",
		"shiplap");

	bReturn = CreateGLTexture(
		"textures/bricks.JPG",
		"brick");

	bReturn = CreateGLTexture(
		"textures/Wood_mantle.JPG",
		"mantle");

	bReturn = CreateGLTexture(
		"textures/black_metal.JPG",
		"metal");

	bReturn = CreateGLTexture(         
		"textures/black_metal2.JPG",
		"metal2");

	bReturn = CreateGLTexture(
		"textures/pine_bark.JPG",
		"bark");
	
	bReturn = CreateGLTexture(
		"textures/Tree_end.JPG", 
		"tree_end");

	bReturn = CreateGLTexture(
		"textures/rusticwood.JPG",
		"rusticwood");

	bReturn = CreateGLTexture(
		"textures/Leaf.JPG",
		"leaf");

	bReturn = CreateGLTexture(
		"textures/BLUEY.JPG",
		"cartoon");

	BindGLTextures();
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
	// load the texture image files for the textures applied
	// to objects in the 3D scene
	LoadSceneTextures();
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->DrawExtraTorusMesh1();
	m_basicMeshes->DrawExtraTorusMesh2();

		
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	RenderWall();
	RenderFireBox();
	RenderTrees();
	RenderWoodenBowl();
	/****************************************************************/

	//Plane for Floor surface
	//set the XYZ scale for the mesh
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.627, 0.322, 0.176, 1);
	SetShaderTexture("floor");
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// Baseboards for scene.
		//set the XYZ scale for the mesh
	

	scaleXYZ = glm::vec3(40.0f, 0.5f, 0.1f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(0.0f, 0.25f, -9.95f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	/****************************************************************/

	// Baseboards for scene.
		//set the XYZ scale for the mesh


	scaleXYZ = glm::vec3(5.25f, 0.5f, 0.1f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(-6.03f, 0.25f, -6.87f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	/****************************************************************/

	// Baseboards for scene.
		//set the XYZ scale for the mesh


	scaleXYZ = glm::vec3(5.25f, 0.5f, 0.1f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(6.03f, 0.25f, -6.87f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	//Outset Fireplace wall strucure box
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(12.0f, 10.0f, 6.0f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(0.0f, 11.0f, -7.0f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.961, 0.400, 0.100, 1); //(0.961, 0.961, 0.961, 1);
	SetShaderTexture("shiplap");
	SetShaderMaterial("shiplap");
	SetTextureUVScale(2.0, 2.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//Outset Fireplace wall strucure box (Lower left of fireplace)
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.75f, 6.0f, 6.0f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(-4.6f, 3.0f, -7.0f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.500, 0.600, 0.200, 1);// Change colors to same as rest of enclosure when final placemnt decided
	SetShaderTexture("shiplap");
	SetShaderMaterial("shiplap");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	//Outset Fireplace wall strucure box (Lower right of fireplace)
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.75f, 6.0f, 6.0f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(4.6f, 3.0f, -7.0f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.500, 0.600, 0.200, 1); // Change colors to same as rest of enclosure when final placemnt decided
	SetShaderTexture("shiplap");
	SetShaderMaterial("shiplap");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	
	/****************************************************************/

	// Verticle corner trim for fireplace outside corner left side.
		//set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 16.0f, 0.5f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(-5.9f, 8.0f, -4.0f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	// Verticle corner trim for fireplace outside corner left side.
		//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 16.0f, 0.5f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(5.9f, 8.0f, -4.0f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	// Verticle corner trim for fireplace Inside back corner left side.
		//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 16.0f, 0.5f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(-5.9f, 8.0f, -9.75f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	// Verticle corner trim for fireplace Inside back corner right side.
		//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 16.0f, 0.5f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(5.9f, 8.0f, -9.75f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	//Mantle
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 1.0f, 2.0f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(0.0f, 8.0f, -3.0f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.545, 0.271, 0.075, 1);
	SetShaderTexture("mantle");
	SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

//Box For Television(Black Outer trim)
//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(9.0f, 5.0f, 0.25f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(0.0f, 11.5f, -3.6f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 0, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	//Box For Television(Screen)
//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.75f, 4.75f, 0.25f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(0.0f, 11.5f, -3.59f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.01, 0.01, 0.01, 1);
	//SetShaderTexture("cartoon");
	SetShaderMaterial("metal");  //Image used from www.wallpapercave.com
	SetTextureUVScale(1, 1);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/****************************************************************/

	//Sphere for snowman base
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.21f, 0.21f, 0.17f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(-3.5f, 8.6f, -2.6f); //(2.0f, 2.0f, 2.0f);

	//set the transformation into memmory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.9, 0.9, 0.9, 1);
	SetShaderMaterial("metal");
	//SetShaderTexture("wporcelain");
	//Draw the mesh with the transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	//Sphere for snowman abdomen.
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.15f, 0.15f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(-3.5f, 8.74f, -2.6f);

	//set the transformation into memmory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.9, 0.9, 0.9, 1);
	SetShaderMaterial("metal");
	//SetShaderTexture("wporcelain");
	//Draw the mesh with the transformation values
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	//Snowman Head.
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.11f, 0.11f, 0.11f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(-3.5f, 8.87f, -2.6f);

	//set the transformation into memmory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.9, 0.9, 0.9, 1);
	SetShaderMaterial("metal");
	
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	//Snowman Hat main.
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.09f, 0.13f, 0.09f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(-3.5f, 8.94f, -2.6f);

	//set the transformation into memmory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.01, 0.01, 0.01, 1);
	SetShaderMaterial("metal");

	//Draw the mesh with the transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//Snowman Hat Brim.
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 0.1f, 0.03f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(-3.5f, 8.94f, -2.6f);  //2.37

	//set the transformation into memmory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.01, 0.01, 0.01, 1);
	SetShaderMaterial("metal");
	//Draw the mesh with the transformation values
	m_basicMeshes->DrawTorusMesh();
}

void SceneManager::RenderFireBox()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//Back Wall Plane of Fireplace
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.5f, 6.0f, 3.0f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(0.0f, 3.0f, -7.5f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.00, 0.00, 0.00, 1);
	SetShaderTexture("brick");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/

	//left Wall Plane of Fireplace Angled
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 0.0f, 3.0f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 70.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(-2.4f, 3.0f, -6.84f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.200, 0.200, 0.100, 1);
	SetShaderTexture("brick");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/

	//righy Wall Plane of Fireplace Angled
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 0.0f, 3.0f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = -70.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(2.4f, 3.0f, -6.84f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.200, 0.200, 0.100, 1);
	SetShaderTexture("brick");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/
	//Fireplace Base Box
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.5f, 1.0f, 5.0f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(0.0f, 0.5f, -7.00f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("metal");
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	//Fireplace Top Box
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.5f, 1.0f, 5.0f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(0.0f, 5.5f, -7.00f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0, 0, 0, 1);
	SetShaderTexture("metal");
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/***************************************************************/
	/* Fire place trim parts                                       */
	/***************************************************************/

	//Black bottom metallic fire place trim.

	scaleXYZ = glm::vec3(7.0f, 1.0f, 0.45f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(0.0f, 0.5f, -4.3f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.100, 0.171, 0.100, 1);
	SetShaderTexture("metal2");
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*************************************************************/

	//Black top metallic fire place trim.

	scaleXYZ = glm::vec3(7.0f, 1.0f, 0.45f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(0.0f, 5.5f, -4.3f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.100, 0.171, 0.100, 1);
	SetShaderTexture("metal2");
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*************************************************************/

	/*************************************************************/

	//Black left side metallic fire place trim.

	scaleXYZ = glm::vec3(5.0f, 0.10f, 0.45f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(-3.2f, 3.5f, -4.3f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.100, 0.171, 0.100, 1);
	SetShaderTexture("metal2");
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*************************************************************/

	//Black right side metallic fire place trim.

	scaleXYZ = glm::vec3(5.0f, 0.10f, 0.45f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(3.2f, 3.5f, -4.3f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.100, 0.171, 0.100, 1);
	SetShaderTexture("metal2");
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*************************************************************/

	//Black Log holder left

	scaleXYZ = glm::vec3(0.8f, 0.2f, 0.2f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 180.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(-1.5f, 1.5f, -6.0f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.100, 0.171, 0.100, 1);
	SetShaderTexture("metal2");
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfTorusMesh();
	/*************************************************************/

	//Black Log holder base left

	scaleXYZ = glm::vec3(0.5f, 0.2f, 0.2f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(-1.5f, 1.0f, -6.0f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.100, 0.171, 0.100, 1);
	SetShaderTexture("metal2");
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfTorusMesh();

	/*************************************************************/

	//Black Log holder right

	scaleXYZ = glm::vec3(0.8f, 0.2f, 0.2f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 180.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(1.5f, 1.5f, -6.0f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.100, 0.171, 0.100, 1);
	SetShaderTexture("metal2");
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfTorusMesh();
	/*************************************************************/

	//Black Log holder base right

	scaleXYZ = glm::vec3(0.5f, 0.2f, 0.2f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(1.5f, 1.0f, -6.0f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.100, 0.171, 0.100, 1);
	SetShaderTexture("metal2");
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfTorusMesh();

	/****************************************************************/

	/****************************************************************/
	/*                    Logs for Fire Place                       */
	/****************************************************************/

	//front bottonm log

	scaleXYZ = glm::vec3(.3f, 3.50f, .3f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(1.8f, 1.6f, -5.6f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.100, 0.171, 0.300, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false, false);

	SetShaderTexture("tree_end");
	SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(true, true, false);

	/***********************************************************/

	//back bottonm log

	scaleXYZ = glm::vec3(.3f, 3.50f, .3f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(1.8f, 1.6f, -6.3f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.100, 0.171, 0.300, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false, false);

	SetShaderTexture("tree_end");
	SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(true, true, false);
	/***********************************************************/

	//Top Diagonal log

	scaleXYZ = glm::vec3(.3f, 3.50f, .3f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 20.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(1.8f, 2.1f, -6.5f);

	//set the transformatinos into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.100, 0.171, 0.300, 1);
	SetShaderTexture("bark");
	SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false, false);

	SetShaderTexture("tree_end");
	SetShaderMaterial("wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(true, true, false);

}
void SceneManager::RenderWall() // restructure code to resemble this
{
	//Back Wall Plane
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 8.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.961, 0.961, 0.961, 1);
	SetShaderTexture("shiplap");
	//SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}
void SceneManager::RenderTrees()
{

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	// Cylinder for left tree base
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 0.25f, 0.25f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(-4.5f, 8.5f, -2.75f);

	//set the transformation into memmory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.961, 0.871, 0.702, 1);

	//Draw the mesh with the transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//Torus for left tree base
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.19f, 0.19f, 0.19f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(-4.5f, 8.75f, -2.75f);

	//set the transformation into memmory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.961, 0.871, 0.702, 1);

	//Draw the mesh with the transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	//Cone for left tree foliage
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.5f, 0.5f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(-4.5f, 8.75f, -2.75f);

	//set the transformation into memmory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	SetShaderTexture("leaf");
	SetTextureUVScale(4, 4);

	//Draw the mesh with the transformation values
	m_basicMeshes->DrawConeMesh();
	/****************************************************************/

	// Cylinder for right tree base
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 0.25f, 0.25f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(4.5f, 8.5f, -2.75f);

	//set the transformation into memmory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.961, 0.871, 0.702, 1);

	//Draw the mesh with the transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//Torus for right tree base
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.19f, 0.19f, 0.19f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(4.5f, 8.75f, -2.75f);

	//set the transformation into memmory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.961, 0.871, 0.702, 1);

	//Draw the mesh with the transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	//Cone for right tree foliage
	//set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 2.5f, 0.5f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set the XYZ positions for the mesh
	positionXYZ = glm::vec3(4.5f, 8.75f, -2.75f);

	//set the transformation into memmory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	SetShaderTexture("leaf");
	SetTextureUVScale(4, 4);
	//Draw the mesh with the transformation values
	m_basicMeshes->DrawConeMesh();
	/****************************************************************/
}


void SceneManager::RenderWoodenBowl()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//set XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.35f, 0.4f, 0.4f); 

	//set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set XYZ positions for the mesh
	positionXYZ = glm::vec3(0.0f, 8.75f, -2.9f);

	//set transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	
	//SetShaderColor(1.0, 1.0, 1.0, 1);
	SetShaderTexture("rusticwood");
	SetShaderMaterial("wood");
	//m_basicMeshes->DrawExtraTorusMesh1();
	//m_basicMeshes->DrawExtraTorusMesh2();
	m_basicMeshes->DrawTorusMesh();

	/************************************************************************/

	//set XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.7f, 0.2f, 0.5f);

	//set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//set XYZ positions for the mesh
	positionXYZ = glm::vec3(0.0f, 8.7f, -2.9f); //(0.0f, 2.95f, 4.0f)

	//set transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1.0, 1.0, 1.0, 1);
	SetShaderTexture("rusticwood");
	SetShaderMaterial("wood");
	
	m_basicMeshes->DrawHalfSphereMesh();
}
