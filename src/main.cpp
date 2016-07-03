#include "ExampleApplication.h"
#include "ExampleFrameListener.h"
#include "OgrePlatform.h"
#include "windows.h"
#include "OgreGridMesh.h"
#include "OgreGridRenderable.h"
#include "Ogre.h"
#ifdef _USE_SKYX_
#include "SkyX.h"
#endif

using namespace Ogre;

void createSphere(const std::string& strName, const float r, const int nRings=16, const int nSegments=16);

void LOD_init(const MapDimensions& mapInfo, int lodLevelCount, int gridDim, float morphStartRatio, const char* heightmapName, const char* hmap2Name);
void LOD_deinit();
void LOD_frameStarted(Ogre::SceneManager* scnMgr, const Ogre::Camera& cam);
float getLODSqRange(size_t lodLevel);

void OgreUpdateHeightmapBlendRatio(float ratio);

#ifdef _USE_SKYX_
void setSkyXPreset(int presetNo, SkyX::SkyX* skyX, SkyX::BasicController* controller, Ogre::Camera* camera);
void getSkyXAtmosphereOptinos(int index, SkyX::AtmosphereManager::Options* options);
#endif
static std::vector<SceneNode*> _lodRangeSphereNodes;
static bool _showRangeSheres = false;

#ifdef _USE_SKYX_
SkyX::SkyX* _skyX = 0;
SkyX::BasicController* _skyXBasicController = 0;
SkyX::AtmosphereManager::Options _skyXAtmosphereOptions;
#endif

static void setLodSphereVisible(bool visible)
{
	for(auto i: _lodRangeSphereNodes)
	{
		i->setVisible(visible);
	}
}

static void setLodSpherePosition(const Vector3& pos)
{
	int l = 0;
	for(auto i: _lodRangeSphereNodes)
	{
		float r = sqrtf(getLODSqRange(l++));
		i->setScale(r, r, r);
		i->setPosition(pos);
	}
}

class TestFrameListener : public ExampleFrameListener
{
	SceneManager* mSceneMgr;
	bool trace_main_camera;
	Camera* LOD_camera;
	// assuming the default value to be 1.0 in GPU
	int hmapBlendRatio = 100; // = 1.0f

	GpuProgramParametersSharedPtr VPparams;
	GpuProgramParametersSharedPtr FPparams;
public:
	TestFrameListener(SceneManager* scnMgr, RenderWindow* win, Camera* cam)
		: ExampleFrameListener(win, cam), mSceneMgr(scnMgr), trace_main_camera(true)
	{
		LOD_camera = scnMgr->createCamera("LODCam");
	}

	virtual bool frameStarted(const FrameEvent& evt)
	{
		Camera* cam = trace_main_camera ? mCamera : LOD_camera;
		LOD_frameStarted(mSceneMgr, *cam);

		if (_showRangeSheres)
		{
			setLodSpherePosition(cam->getPosition());
		}
#ifdef _USE_SKYX_
		_skyXAtmosphereOptions.HeightPosition = cam->getPosition().y / 20000.0f;
		_skyX->getAtmosphereManager()->setOptions(_skyXAtmosphereOptions);
#endif
		return true;
	}

	virtual bool processUnbufferedKeyInput(const FrameEvent& evt)
	{
		if (!ExampleFrameListener::processUnbufferedKeyInput(evt))
			return false;
		if (mKeyboard->isKeyDown(OIS::KC_C) && mTimeUntilNextToggle <= 0)
		{
			trace_main_camera = !trace_main_camera;
			if (!trace_main_camera)
			{
				LOD_camera->setPosition(mCamera->getPosition());
				LOD_camera->setDirection(mCamera->getDirection());
				LOD_camera->setNearClipDistance(mCamera->getNearClipDistance());
				LOD_camera->setFarClipDistance(mCamera->getFarClipDistance());
				LOD_camera->setFOVy(mCamera->getFOVy());
			}
			mTimeUntilNextToggle = 1;
		}
		if (mKeyboard->isKeyDown(OIS::KC_B) && mTimeUntilNextToggle <= 0)
		{
			static bool showBB = false;
			showBB = !showBB;
			mSceneMgr->showBoundingBoxes(showBB);
			mTimeUntilNextToggle = 1;
		}
		if (mKeyboard->isKeyDown(OIS::KC_V) && mTimeUntilNextToggle <= 0)
		{
			_showRangeSheres = !_showRangeSheres;
			setLodSphereVisible(_showRangeSheres);
			mTimeUntilNextToggle = 1;
		}

		if (mKeyboard->isKeyDown(OIS::KC_0) && mTimeUntilNextToggle <= 0)
		{
			hmapBlendRatio -= 10;
			if (hmapBlendRatio < 0)
				hmapBlendRatio = 0;
			OgreUpdateHeightmapBlendRatio(hmapBlendRatio * 0.01f);
			mTimeUntilNextToggle = 0.5f;
		}

		if (mKeyboard->isKeyDown(OIS::KC_1) && mTimeUntilNextToggle <= 0)
		{
			hmapBlendRatio += 10;
			if (hmapBlendRatio > 100)
				hmapBlendRatio = 100;
			OgreUpdateHeightmapBlendRatio(hmapBlendRatio * 0.01f);
			mTimeUntilNextToggle = 0.5f;
		}
			
		return true;
	}

	virtual bool processUnbufferedMouseInput(const FrameEvent& evt)
	{
		if (mKeyboard->isKeyDown(OIS::KC_LCONTROL))
		{
			const OIS::MouseState &ms = mMouse->getMouseState();
#ifdef _USE_SKYX_
			Vector3 skyTime = _skyXBasicController->getTime();
			skyTime.x += ms.X.rel * 0.01f;
			_skyXBasicController->setTime(skyTime);
			Vector3 sunDir = _skyXBasicController->getSunDirection();
#else
			Vector3 sunDir = Vector3(0, 1, 0);
#endif
			Light* light = mSceneMgr->getLight("DirectionalLight");
			light->setDirection(-sunDir);
#if 0
			Vector3 lightDir = light->getDirection();
			Vector3 camDir = mCamera->getDirection();
			camDir.y = 0;
			Quaternion q(Radian(ms.X.rel * 0.003f), camDir);
			lightDir = q * lightDir;
			light->setDirection(lightDir.normalisedCopy());
#endif
		} else
		{
			ExampleFrameListener::processUnbufferedMouseInput(evt);
		}
		return true;
	}
};

class TestApplication : public ExampleApplication
{
public:
	TestApplication() : ExampleApplication()
	{
	}

protected:
	virtual void createFrameListener(void)
	{
		mFrameListener= new TestFrameListener(mSceneMgr, mWindow, mCamera);
		mFrameListener->showDebugOverlay(true);
        mRoot->addFrameListener(mFrameListener);
	}

	void load_mapinfo(int* lodLevel, float *morphRatio, int* gridDim, MapDimensions* map, char* heightmapName, char* heightmap2Name, size_t buflen, Vector3* skyXTime)
	{
#ifdef NORMAL_TEXT_CONFIG
		float nearClip, farClip;
		float camX, camY, camZ;

		FILE *fp = fopen("mapinfo.cfg", "r");
		if (!fp)
			return;
		fscanf(fp, "%f %f", &nearClip, &farClip);
		fscanf(fp, "%f %f %f", &camX, &camY, &camZ);
		fscanf(fp, "%d %d %f", lodLevel, gridDim, morphRatio);
		fscanf(fp, "%f %f %f", &map->MinX, &map->MinY, &map->MinZ);
		fscanf(fp, "%f %f %f", &map->SizeX, &map->SizeY, &map->SizeZ);
		fscanf_s(fp, "%s", heightmapName, buflen);
		fclose(fp);

		mCamera->setPosition(Vector3(camX, camY, camZ));
		mCamera->setDirection(Vector3(0, -1, -1));
		mCamera->setNearClipDistance(nearClip);
		mCamera->setFarClipDistance(farClip);
#else
		ConfigFile cfg;
		cfg.load("mapinfo.ogre.cfg");
		Vector3 campPos = StringConverter::parseVector3(cfg.getSetting("Camera Position"));
		float camNearDist = StringConverter::parseReal(cfg.getSetting("Camera Near Dist"));
		float camFarDist = StringConverter::parseReal(cfg.getSetting("Camera Far Dist"));
		*lodLevel = StringConverter::parseInt(cfg.getSetting("LOD levels"));
		*gridDim = StringConverter::parseInt(cfg.getSetting("Grid Dimension"));
		*morphRatio = StringConverter::parseReal(cfg.getSetting("Morph Ratio"));
		Vector3 mapMinPos = StringConverter::parseVector3(cfg.getSetting("Map starting position"));
		map->MinX = mapMinPos.x;
		map->MinY = mapMinPos.y;
		map->MinZ = mapMinPos.z;
		Vector3 mapSize = StringConverter::parseVector3(cfg.getSetting("Map Size"));
		map->SizeX = mapSize.x;
		map->SizeY = mapSize.y;
		map->SizeZ = mapSize.z;
		String hmap1Name = cfg.getSetting("Heightmap");
		strcpy_s(heightmapName, buflen, hmap1Name.c_str());
		String hmap2Name = cfg.getSetting("Heightmap2");
		strcpy_s(heightmap2Name, buflen, hmap2Name.c_str());
		*skyXTime = StringConverter::parseVector3(cfg.getSetting("SkyX Time"));

		mCamera->setPosition(campPos);
		mCamera->setDirection(Vector3(0, -1, -1));
		mCamera->setNearClipDistance(camNearDist);
		mCamera->setFarClipDistance(camFarDist);
#endif
		unsigned short nMaxLODSize = 1 << (*lodLevel - 1);
		map->nGridX = nMaxLODSize;
		map->nGridZ = nMaxLODSize;
		map->gridSizeX = map->SizeX / map->nGridX;
		map->gridSizeZ = map->SizeZ / map->nGridZ;
	}

	virtual void createScene(void)
	{
		createSphere("mySphereMesh", 1);
		Entity* sphereEntity = mSceneMgr->createEntity("mySphereEntity", "mySphereMesh");
		SceneNode* sphereNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
		sphereEntity->setMaterialName("material_name_goes_here");
		sphereNode->attachObject(sphereEntity);
		sphereNode->setPosition(0, 50, 0);
		sphereNode->setScale(30, 30, 30);

		Light* light = mSceneMgr->createLight("DirectionalLight");
		light->setType(Light::LT_DIRECTIONAL);
		
		MapDimensions mapInfo = 
		{
			-1024, 0, -1024,
			 2048, 10, 2048,
			 0, 0
		};
		int lodLevel = 8;
		int gridDim = 5;
		char heightmapName[256];
		char heightmap2Name[256];

		float morphStartRatio = 0.66f;
		Vector3 skyXTime;

		load_mapinfo(&lodLevel, &morphStartRatio, &gridDim, &mapInfo, heightmapName, heightmap2Name, sizeof(heightmapName), &skyXTime);
		LOD_init(mapInfo, lodLevel, gridDim, morphStartRatio, heightmapName, heightmap2Name);
		OgreGridRenderable::initOgreGridRenderable(gridDim);
		OgreGridRenderable::addLight(light);

		createSphere("BoundingSphere", 1);
		for(int l = 0; l < lodLevel; l++)
		{
			String indexStr = StringConverter::toString(l);
			Entity* entity = mSceneMgr->createEntity("BoundingSpehereEnt"+indexStr, "mySphereMesh");
			entity->setMaterialName("BaseWhiteNoLighting");
			SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
			node->attachObject(entity);
			node->setVisible(false);
			_lodRangeSphereNodes.push_back(node);
		}
#ifdef _USE_SKYX_
		// SkyX
		_skyXBasicController = new SkyX::BasicController();
		_skyX = new SkyX::SkyX(mSceneMgr, _skyXBasicController);
		_skyX->create();
		_skyX->getVCloudsManager()->getVClouds()->setDistanceFallingParams(Ogre::Vector2(2, -1));
		mRoot->addFrameListener(_skyX);
		mWindow->addListener(_skyX);
		setSkyXPreset(1, _skyX, _skyXBasicController, mCamera);

		_skyXBasicController->setTime(skyXTime);
		Vector3 sunDir = _skyXBasicController->getSunDirection();
		light->setDirection(-sunDir);
#else
		Vector3 sunDir = Vector3(0, 1, 0);
		light->setDirection(-sunDir);
#endif
	}

	virtual void destroyScene(void)
	{
		OgreGridRenderable::deinitOgreGridRenderable();
		LOD_deinit();
	}
};

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR cmdLine, INT)
{
	int argc = __argc;
	char** argv = __argv;
	try
	{
		TestApplication testApp;
		testApp.go();
	}
	catch (Ogre::Exception& e)
	{
		MessageBoxA(NULL, e.getFullDescription().c_str(), "An exception has occurred!", MB_ICONERROR | MB_TASKMODAL);
	}
	return 0;
}