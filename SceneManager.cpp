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
	for (int i = 0; i < m_loadedTextures; ++i)
		if (m_textureIDs[i].ID != 0)
			glDeleteTextures(1, &m_textureIDs[i].ID);
	m_loadedTextures = 0;
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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** The code in the methods BELOW is for preparing and     ***/
/*** rendering the 3D replicated scenes.                    ***/
/**************************************************************/


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
		"Debug/textures/woodesk.jpg", "desk");

	bReturn = CreateGLTexture(
		"Debug/textures/mugbody.jpg", "mugbody");

	bReturn = CreateGLTexture(
		"Debug/textures/mugholder.jpg", "mugholder");

	bReturn = CreateGLTexture(
		"Debug/textures/coffeetop1.jpg", "coffee");

	bReturn = CreateGLTexture(
		"Debug/textures/keyboard.jpg", "keyboard");

	bReturn = CreateGLTexture(
		"Debug/textures/Screentexture.jpg", "screen");

	bReturn = CreateGLTexture(
		"Debug/textures/Macbook.jpg", "macbook");

	bReturn = CreateGLTexture(
		"Debug/textures/lamp.jpg", "lamp");

	bReturn = CreateGLTexture(
		"Debug/textures/wall.jpg", "wall");

	bReturn = CreateGLTexture(
		"Debug/textures/mouse.jpg", "mouse");

	bReturn = CreateGLTexture(
		"Debug/textures/mousepad.png", "mousepad");




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

	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f);
	plasticMaterial.ambientStrength = 0.4f;
	plasticMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	plasticMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	plasticMaterial.shininess = 12.0f;
	plasticMaterial.tag = "plastic";

	m_objectMaterials.push_back(plasticMaterial);

	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	metalMaterial.ambientStrength = 0.3f;
	metalMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	metalMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	metalMaterial.shininess = 17.0;
	metalMaterial.tag = "metal";

	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL cementMaterial;
	cementMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cementMaterial.ambientStrength = 0.06f;
	cementMaterial.diffuseColor = glm::vec3(0.42f, 0.42f, 0.42f);
	cementMaterial.specularColor = glm::vec3(0.8f, 0.6f, 0.3f);
	cementMaterial.shininess = 8.0f;
	cementMaterial.tag = "cement";


	m_objectMaterials.push_back(cementMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	woodMaterial.ambientStrength = 0.1f;
	woodMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);       
	woodMaterial.specularColor = glm::vec3(0.8f, 0.6f, 0.3f);      
	woodMaterial.shininess = 16.0f;                                
	woodMaterial.tag = "wood";


	m_objectMaterials.push_back(woodMaterial);


	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.2f, 0.3f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	glassMaterial.specularColor = glm::vec3(0.4f, 0.5f, 0.6f);
	glassMaterial.shininess = 35.0f;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL clayMaterial;
	clayMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.3f);
	clayMaterial.ambientStrength = 0.3f;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	clayMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f);
	clayMaterial.shininess = 0.5f;
	clayMaterial.tag = "clay";

	m_objectMaterials.push_back(clayMaterial);

	OBJECT_MATERIAL MousepadMaterial;
	MousepadMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	MousepadMaterial.ambientStrength = 0.3f;
	MousepadMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	MousepadMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f);
	MousepadMaterial.shininess = 2.0f;
	MousepadMaterial.tag = "Mousepad";

	m_objectMaterials.push_back(MousepadMaterial);

}


void SceneManager::SetupSceneLights()
{

	/****************************************************************/
	/*** Light 0 – LEFT WARM WASH ***/
	m_pShaderManager->setVec3Value("lightSources[0].position", -6.8f, 1.10f, -4.25f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.0f, 0.20f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 0.5f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.15f, 0.05f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 10.0f);        
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.12f);        
	/****************************************************************/

	/*** Light 1 – TOP WHITE HALO ***/
	m_pShaderManager->setVec3Value("lightSources[1].position", 0.0f, 10.2f, -4.8f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.10f, 0.12f, 0.16f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.70f, 0.74f, 0.82f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 30.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.65f);
	/****************************************************************/

	/*** Light 2 – RIGHT/BACK WHITE FILL  ***/
	m_pShaderManager->setVec3Value("lightSources[2].position", 16.5f, 3.5f, 3.5f);		   
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.20f, 0.21f, 0.23f);   
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.58f, 0.62f, 0.68f);  
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.62f, 0.66f, 0.72f);  
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 110.0f);              
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.82f);          
	/****************************************************************/
	
	/*** Light 3 – UNDER-DESK STRIP ***/
	m_pShaderManager->setVec3Value("lightSources[3].position", 0.0f, -0.32f, -6.20f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.01f, 0.001f, 0.001f); 
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.80f, 0.28f, 0.02f);  
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.80f, 0.28f, 0.02f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 8.0f);       
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.05f); 


	// enable the use of lighting in the shader
	m_pShaderManager->setBoolValue("bUseLighting", true);



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

	//define the object materials that will be used in the scene
	DefineObjectMaterials();

	//Setting up scene lighting
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
	RenderTable();
	RenderLamp();
	RenderBackdrop();
	RenderLaptop();
	RenderMonitor();
	RenderKeyboard();
	RenderMouse();
	RenderMug();
	RenderMousepad();
}

/****************************************************************
	*  RenderTable()
	*
	*  This method is called to render the shapes for the scene
	*  Table object.
	****************************************************************/
void SceneManager::RenderTable()
{

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 0.3f, 9.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -0.30f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//Use the desk texture
	SetShaderTexture("desk");

	//Scale the texture to fit the desk
	SetTextureUVScale(1, 1);

	// Use the wood material
	SetShaderMaterial("wood");

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();

}

/****************************************************************
*  RenderBackdrop()
*
*  This method is called to render the shapes for the scene
*  backdrop object.
****************************************************************/
void SceneManager::RenderBackdrop()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the scale for the mesh
	scaleXYZ = glm::vec3(13.0f, 0.3f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.0f, -5.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Use the wall texture
	SetShaderTexture("wall");

	// Use the cement material
	SetShaderMaterial("cement");

	// Set the UV scale for the texture mapping
	SetTextureUVScale(2, 1);

	// draw the mesh
	m_basicMeshes->DrawPlaneMesh();
}

/****************************************************************
*  RenderLamp()
*
*  This method is called to render the shapes for the scene
*  Lamp object.
/****************************************************************/
void SceneManager::RenderLamp()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//Sphere - Lamp Body *******/

	// Declare scale for the mesh
	scaleXYZ = glm::vec3(1.8f, 0.7f, 1.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 0.9f, -3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// Set the shader texture for the lamp body
	SetShaderTexture("lamp");

	// Set the texture UV scale for the lamp body
	SetTextureUVScale(1.0, 1.0); // Scale the texture to fit the sphere

	// Set the shader material for the lamp body
	SetShaderMaterial("glass");

	// draw the mesh
	m_basicMeshes->DrawSphereMesh();


	/****************************************************************/

	// Cylinder - Lamp Pole *******/

	// Declare scale for the mesh
	scaleXYZ = glm::vec3(1.4f, 0.9f, 1.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 0.0f, -3.0f); // Base on ground

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader color for the lamp pole
	SetShaderColor(0.3f, 0.2f, 0.0f, 1.0f);

	// Set the shader material for the lamp body
	SetShaderMaterial("plastic");

	// draw the mesh
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/*** Lamp Switch - Box ***/

	// Declare scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 0.15f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.5f, 0.3f, -1.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader color for the lamp switch
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	// Set the shader material for the lamp body
	SetShaderMaterial("plastic");

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();
}

/****************************************************************
*  RenderMug()
*
*  This method is called to render the shapes for the scene
*  Mug object.
****************************************************************/
void SceneManager::RenderMug()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	// Declare scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 1.5f, 0.5f); // Tall cylinder for body

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 0.1f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader color or texture for the mug body
	SetShaderTexture("mugbody"); // Body texture


	// Set the shader material for the mug body
	SetShaderMaterial("clay");

	// Set the texture UV scale to fit the cylinder
	SetTextureUVScale(1.0, 5.0);

	//This draws the mug body cylinder side 
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	//This draws the mug body cylinder top 
	// set the texture for the top of the mug

	SetShaderTexture("coffee");

	// Set the texture UV scale for the top
	SetTextureUVScale(0.7, 0.7);

	// Draw the top of the mug as a flat cylinder
	m_basicMeshes->DrawCylinderMesh(true, false, false);

	/****************************************************************/

	// Mug Handle - Torus *******/

	// Declare scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f; // Rotate to align like a handle
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.7f, 0.8f, 3.0f); // Positioned to side

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader texture for the mug handle
	SetShaderTexture("mugholder");

	// Set the shader material for the mug handle
	SetShaderMaterial("clay");

	// Set the texture UV scale to fit the torus
	SetTextureUVScale(5.0, 1.0);

	// Draw the torus mesh for the mug handle
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/
}

/****************************************************************
*  RenderLaptop()
*
*  This method is called to render the shapes for the scene
*  Laptop object.
****************************************************************/
void SceneManager::RenderLaptop()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Declare scale for the mesh
	scaleXYZ = glm::vec3(2.75f, 2.0f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f; // 
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 1.3f, -4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader color for the macbook
	SetShaderColor(0.3f, 0.3f, 0.3f, 1.0f); // Silver-gray

	// Set the shader material for the mug body
	SetShaderMaterial("metal");

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	// Macbook Plane ( For logo )

	// Declare scale for the mesh
	scaleXYZ = glm::vec3(1.38f, 0.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f; // That makes mesh vertical
	YrotationDegrees = 180.0f; // Rotating logo like on the reference image
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 1.3f, -4.465f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader texture for the macbook plane
	SetShaderTexture("macbook");

	// Set the shader material for the macbook plane
	SetShaderMaterial("metal");

	// Set the texture UV scale to fit the plane
	SetTextureUVScale(1.0, 0.95);

	// draw the mesh
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/

	// Macbook Holder ( Bottom Stand )

	// Declare scale for the mesh
	scaleXYZ = glm::vec3(1.1f, 0.1f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f; // 
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 0.3f, -4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader color for the macbook holder
	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);

	// Set the shader material for the macbook holder
	SetShaderMaterial("plastic");

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	// Macbook Holder ( Vertical Stand )

	// Declare scale for the mesh
	scaleXYZ = glm::vec3(1.1f, 0.4f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f; // 
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 0.5f, -4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader color for the macbook holder
	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);

	// Set the shader material for the macbook holder
	SetShaderMaterial("plastic");

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();
}

/****************************************************************
*  RenderMonitor()
*
*  This method is called to render the shapes for the scene
*  Monitor object.
****************************************************************/
void SceneManager::RenderMonitor()
{

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Monitor - Screen Box ***/

	// Declare scale for the mesh
	scaleXYZ = glm::vec3(8.5f, 4.5f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.7f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader color for the monitor screen
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);

	// Set the shader material for the monitor screen
	SetShaderMaterial("metal");

	//draw the mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	/*** Monitor - Screen Plane ***/

	// Declare scale for the mesh
	scaleXYZ = glm::vec3(4.1f, 4.5f, 2.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.7f, -4.14f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader texture for the monitor plane
	SetShaderTexture("screen");

	// Set the shader material for the monitor screen
	SetShaderMaterial("glass");

	// Set the texture UV scale to fit the plane
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/
	/*** Monitor - Stand (Pole) ***/

	// Declare scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 2.5f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.1f, 0.3f, -4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader color for the monitor stand pole
	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);

	// Set the shader material for the monitor screen
	SetShaderMaterial("metal");

	// draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	/*** Monitor - Stand (Base Plate) ***/

	// Declare scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.1f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.1f, 0.2f, -4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader color for the monitor stand base plate
	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);

	// Set the shader material for the monitor screen
	SetShaderMaterial("metal");

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/*** Monitor - Light Bar ***/

	// Declare scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 0.1f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 6.2f, -4.17f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader color for the monitor light bar
	SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);

	// Set the shader material for the mug body
	SetShaderMaterial("plastic");

	// draw the mesh

	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/*** Monitor - Light Bar Small Box (Center Piece) ***/

	// Declare scale for the mesh
	scaleXYZ = glm::vec3(0.6f, 0.3f, 0.2f); // Centered box

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 6.10f, -4.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader material for the mug body
	SetShaderMaterial("plastic");

	// Set the shader color for the monitor light bar small box
	SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();
}

/**************************************************************
*  RenderMouse()
*
*  This method is called to render the shapes for the scene
*  Mouse object.
***************************************************************/
void SceneManager::RenderMouse()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	// declare the scale for the mouse mesh
	scaleXYZ = glm::vec3(0.5f, 0.4f, 0.8f); // Rounded upper body

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.5f, 0.4f, 2.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader texture for the mouse
	SetShaderTexture("mouse"); // Use the screen texture

	// Set the texture UV scale to fit the mouse
	SetTextureUVScale(1.0, 1.0);

	// Set the shader material for the mouse
	SetShaderMaterial("plastic");

	// draw the mesh
	m_basicMeshes->DrawSphereMesh();
}

/****************************************************************
*  RenderKeyboard()
*
*  This method is called to render the shapes for the scene
*  Keyboard object.
****************************************************************/
void SceneManager::RenderKeyboard()
{

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Keyboard Plane ***/

	// Declare scale for the mesh
	scaleXYZ = glm::vec3(1.9f, 0.0f, 0.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.46f, 2.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// Set the shader color for the keyboard plane
	SetShaderTexture("keyboard");

	// Set the shader material for the keyboard plane
	SetShaderMaterial("plastic");

	// Set the texture UV scale to fit the keyboard plane
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/

	/*** Keyboard Base ***/

	// Declare scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 0.2f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.35f, 2.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the shader color for the keyboard
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);

	// Set the shader material for the keyboard plane
	SetShaderMaterial("plastic");

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();

}

/****************************************************************
*  RenderMousepad()
*
*  This method is called to render the shapes for the scene
*  Mousepad object.
****************************************************************/
void SceneManager::RenderMousepad()
{

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	// Declare scale for the mesh
	scaleXYZ = glm::vec3(9.5f, 0.030f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.2f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// Set the shader material for the mousepad
	SetShaderMaterial("Mousepad");

	// Set Shader texture
	SetShaderTexture("mousepad");

	// draw the mesh
	m_basicMeshes->DrawBoxMesh();

}
