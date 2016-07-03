#include "OgreAxisAlignedBox.h"
#include "OgreCamera.h"
#include "OgreSceneManager.h"
#include "OgreGridRenderable.h"
#include "OgreImage.h"

enum LODSelectResult
{
    Undefined,
    OutOfFrustum,
    OutOfRange,
    Selected
};

enum IntersectType
{
   Outside,
   Intersect,
   Inside
};

static MapDimensions _mapInfo;

static int _LODLevelCount;
static unsigned short _nMaxLODSize;

static std::vector<float> _lodRangeDistRatios;
static std::vector<float> _lodSqRanges;

struct HeightMinMax
{
	unsigned short minY;
	unsigned short maxY;
};

struct MorphConstants
{
	float morphStart;
	float morphEnd;
	float const1;
	float const2;
};
static std::vector<MorphConstants> _morphConsts;
static float _morphStartRatio = 0.66f;

static const int _maxSelectionCount = 4096;
static int _ogreGridRenderableCount = 0;
static Ogre::OgreGridRenderable _ogreGridRenderables[_maxSelectionCount];

static const char* _baseMaterialName = "OgreGridRenderableMaterial";
static Ogre::MaterialPtr _material;
static size_t _nMaterial = 0;

static unsigned int _heightmap_width;
static unsigned int _heightmap_height;

static const HeightMinMax& getHeightMinMax(int lodLevel, int x, int z, bool shrink);

const MapDimensions& LOD_getMapInfo()
{
	return _mapInfo;
}

float getLODSqRange(size_t lodLevel)
{
	assert(lodLevel < _lodSqRanges.size());
	return _lodSqRanges[lodLevel];
}

static inline float getWorldHeight(unsigned short y)
{
	return y * _mapInfo.SizeY / 65535.0f + _mapInfo.MinY;
}

void GetWorldAABB(Ogre::AxisAlignedBox& aabb, const MapDimensions& mapInfo, int LODLevel, unsigned int x, unsigned int z, unsigned short size, float minY, float maxY)
{
	float minx, minz, maxx, maxz;
	float xcoeff = mapInfo.SizeX/mapInfo.nGridX;
	float zcoeff = mapInfo.SizeZ/mapInfo.nGridZ;
	minx = mapInfo.MinX + x * xcoeff;
	maxx = mapInfo.MinX + (x + size) * xcoeff;
	minz = mapInfo.MinZ + z * zcoeff;
	maxz = mapInfo.MinZ + (z + size) * zcoeff;

	aabb.setMinimum(minx, minY, minz);
	aabb.setMaximum(maxx, maxY, maxz);
}

void GetCornerPoints(Ogre::Vector3 corners[], const Ogre::AxisAlignedBox& aabb)
{
	Ogre::Vector3 Min = aabb.getMinimum();
	Ogre::Vector3 Max = aabb.getMaximum();

	corners[0].x = Min.x;
	corners[0].y = Min.y;
	corners[0].z = Min.z;

	corners[1].x = Min.x;
	corners[1].y = Max.y;
	corners[1].z = Min.z;

	corners[2].x = Max.x;
	corners[2].y = Min.y;
	corners[2].z = Min.z;

	corners[3].x = Max.x;
	corners[3].y = Max.y;
	corners[3].z = Min.z;

	corners[4].x = Min.x;
	corners[4].y = Min.y;
	corners[4].z = Max.z;

	corners[5].x = Min.x;
	corners[5].y = Max.y;
	corners[5].z = Max.z;

	corners[6].x = Max.x;
	corners[6].y = Min.y;
	corners[6].z = Max.z;

	corners[7].x = Max.x;
	corners[7].y = Max.y;
	corners[7].z = Max.z;

	corners[8] = aabb.getCenter();
}

IntersectType TestInBoundingPlanes(const Ogre::AxisAlignedBox& aabb, const Ogre::Camera& cam)
{
	Ogre::Vector3 corners[9];
	GetCornerPoints(corners, aabb);
	
	Ogre::Vector3 boxSize = aabb.getSize();
	float size = boxSize.length();

	const Ogre::Plane* planes = cam.getFrustumPlanes();

	// test box's bounding sphere against all planes - removes many false tests, adds one more check
	for(int p = 0; p < 6; p++) 
	{
		float centDist = planes[p].getDistance(corners[8]);
		if( centDist < -size/2 )
			return Outside;
	}

	int totalIn = 0;
	size /= 6.0f; //reduce size to 1/4 (half of radius) for more precision!! // tweaked to 1/6, more sensible

	// test all 8 corners and 9th center point against the planes
	// if all points are behind 1 specific plane, we are out
	// if we are in with all points, then we are fully in
	for(int p = 0; p < 6; p++) 
	{
		int inCount = 9;
		int ptIn = 1;

		for(int i = 0; i < 9; ++i) 
		{
			// test this point against the planes
			float distance = planes[p].getDistance(corners[i]);
			if (distance < -size) 
			{
				ptIn = 0;
				inCount--;
			}
		}

		// were all the points outside of plane p?
		if (inCount == 0) 
		{
			return Outside;
		}

		// check if they were all on the right side of the plane
		totalIn += ptIn;
	}

	if( totalIn == 6 )
		return Inside;

	return Intersect;
}

LODSelectResult LOD_select(const Ogre::Camera& cam, bool parentInFrustum, unsigned int x, unsigned int z, unsigned short size, int LODLevel)
{
	Ogre::AxisAlignedBox aabb;

	const HeightMinMax& h = getHeightMinMax(LODLevel, x, z, true);
	float minY = getWorldHeight(h.minY);
	float maxY = getWorldHeight(h.maxY);
	GetWorldAABB(aabb, _mapInfo, LODLevel, x, z, size, minY, maxY);

	IntersectType frustumIt = (parentInFrustum)?(Inside):TestInBoundingPlanes(aabb, cam);
	if( frustumIt == Outside )
		return OutOfFrustum;

	if (aabb.squaredDistance(cam.getPosition()) > _lodSqRanges[LODLevel])
		return OutOfRange;

	LODSelectResult subTLSelRes = Undefined;
	LODSelectResult subTRSelRes = Undefined;
	LODSelectResult subBLSelRes = Undefined;
	LODSelectResult subBRSelRes = Undefined;

	if (LODLevel > 0)
	{
		int nextLODLevel = LODLevel - 1;
		if (aabb.squaredDistance(cam.getPosition()) <= _lodSqRanges[nextLODLevel])
		{
			bool weAreInFrustum = frustumIt == Inside;
			//bool subTLExists, subTRExists, subBLExists, subBRExists;
			//lodSelectInfo.MinMaxMap[LODLevel-1].GetSubNodesExist( minMaxMapX, minMaxMapY, subTLExists, subTRExists, subBLExists, subBRExists );

			unsigned short halfSize = size / 2;
			subTLSelRes = LOD_select( cam, weAreInFrustum, x,            z,            halfSize, nextLODLevel);
			subTRSelRes = LOD_select( cam, weAreInFrustum, x + halfSize, z,            halfSize, nextLODLevel);
			subBLSelRes = LOD_select( cam, weAreInFrustum, x,            z + halfSize, halfSize, nextLODLevel);
			subBRSelRes = LOD_select( cam, weAreInFrustum, x + halfSize, z + halfSize, halfSize, nextLODLevel);
		}
	}
	// We don't want to select sub nodes that are invisible (out of frustum) or are selected;
	// (we DO want to select if they are out of range, since we are in range)
	bool bRemoveSubTL = (subTLSelRes == OutOfFrustum) || (subTLSelRes == Selected);
	bool bRemoveSubTR = (subTRSelRes == OutOfFrustum) || (subTRSelRes == Selected);
	bool bRemoveSubBL = (subBLSelRes == OutOfFrustum) || (subBLSelRes == Selected);
	bool bRemoveSubBR = (subBRSelRes == OutOfFrustum) || (subBRSelRes == Selected);

	// select (whole or in part) unless all sub nodes are selected by child nodes, either as parts of this or lower LOD levels
	if (!(bRemoveSubTL && bRemoveSubTR && bRemoveSubBL && bRemoveSubBR))
	{
		// add this node information
		if (_ogreGridRenderableCount < _maxSelectionCount)
		{
			_ogreGridRenderables[_ogreGridRenderableCount++].setNodeInfo(NodeInfo(x, z, size, h.minY, h.maxY, LODLevel, !bRemoveSubTL, !bRemoveSubTR, !bRemoveSubBL, !bRemoveSubBR));
		}
		return Selected;
	}
	// if any of child nodes are selected, then return selected - otherwise all of them are out of frustum, so we're out of frustum too
	if( (subTLSelRes == Selected) || (subTRSelRes == Selected) || (subBLSelRes == Selected) || (subBRSelRes == Selected) )
		return Selected;
	else
		return OutOfFrustum;
}

static Ogre::SceneNode* _LOD_node = 0;
static const Ogre::String _objBaseName("OGR");

void LOD_getMorphConsts(int lodLevel, float consts[])
{
	consts[2] = _morphConsts[lodLevel].const1;
	consts[3] = _morphConsts[lodLevel].const2;
}

static void LOD_updateCustomGpuParams(Ogre::SceneManager* scnMgr, const Ogre::Camera& cam)
{
	Ogre::Pass* pass = _material->getTechnique(0)->getPass(0);
	Ogre::GpuProgramParametersSharedPtr vProgram = pass->getVertexProgramParameters();
	vProgram->setNamedConstant("cameraPos", cam.getPosition());
}

// For adding a SceneNode to each grid
#define USE_SAPARATED_NODE

void LOD_frameStarted(Ogre::SceneManager* scnMgr, const Ogre::Camera& cam)
{
	_ogreGridRenderableCount = 0;
	
	float LODNear = cam.getNearClipDistance();
	float LODRange = cam.getFarClipDistance() * 1.5f + LODNear;
	float prevPos = LODNear;
	for( int i = 0; i < _LODLevelCount; i++ ) 
	{
		float range = LODNear + _lodRangeDistRatios[i] * LODRange;
		float morphEnd = _morphConsts[i].morphEnd = range;
		_lodSqRanges[i] = range * range;
		_morphConsts[i].morphStart = prevPos + (morphEnd - prevPos) * _morphStartRatio;
		_morphConsts[i].const2 = 1 / (morphEnd - _morphConsts[i].morphStart);
		_morphConsts[i].const1 = morphEnd * _morphConsts[i].const2;
#ifdef ORIGINAL_CDLOD
		prevPos = _morphConsts[i].morphStart;
#else
		prevPos = morphEnd;
#endif
	}

	LOD_select(cam, false, 0, 0, _nMaxLODSize, _LODLevelCount-1);

	if (_LOD_node == 0)
	{
		_LOD_node = scnMgr->getRootSceneNode()->createChildSceneNode();
	}
#ifdef USE_SAPARATED_NODE
	_LOD_node->removeAndDestroyAllChildren();
#else
	_LOD_node->detachAllObjects();
#endif
	for(int i = 0; i < _ogreGridRenderableCount; i++)
	{
		_ogreGridRenderables[i].setName(_objBaseName + Ogre::StringConverter::toString(i));
#ifdef USE_SAPARATED_NODE
		Ogre::SceneNode* node = _LOD_node->createChildSceneNode();
		node->attachObject(&_ogreGridRenderables[i]);
#else
		_LOD_node->attachObject(&_ogreGridRenderables[i]);
#endif
	}

	LOD_updateCustomGpuParams(scnMgr, cam);
}

const float _LODLevelDistanceRatio = 2.0f;

static std::vector<HeightMinMax *> _heightMinMax;

static const HeightMinMax& getHeightMinMax(int lodLevel, int x, int z, bool shrink = false)
{
	int nX = _nMaxLODSize >> lodLevel;
	assert((size_t)lodLevel < _heightMinMax.size());
	int ix = shrink ? (x * nX / _nMaxLODSize) : x;
	int iz = shrink ? (z * nX / _nMaxLODSize) : z;
	assert(ix < nX && iz < nX);
	return _heightMinMax[lodLevel][iz*nX + ix];
}

void save_h()
{
	FILE* fp = fopen("lod.txt", "w");
	if (!fp)
		return;
	for(int lodLevel=0; lodLevel <_LODLevelCount; lodLevel++)
	{
		unsigned int divider = 1 << lodLevel;
		unsigned int nGridX = _mapInfo.nGridX / divider;
		unsigned int nGridZ = _mapInfo.nGridZ / divider;
		fprintf(fp, "LOD %d\n", lodLevel);
		for(size_t iz=0; iz<nGridZ; iz++)
		{
			for(size_t ix=0; ix<nGridX; ix++)
			{
				const HeightMinMax& h = getHeightMinMax(lodLevel, ix,   iz);
				fprintf(fp, "[%02d %02d] %05d %05d\n", ix, iz, h.minY, h.maxY);
			}
		}
	}
	fclose(fp);
}

void ConstructLODfromHeightmap(const MapDimensions& mapInfo, const char* heightmapName)
{
	// height map analysis
	Ogre::Image heightmapSrc;
	heightmapSrc.load(heightmapName, "General");
	const unsigned int width = heightmapSrc.getWidth();
	const unsigned int height = heightmapSrc.getHeight();
	const unsigned short* pImgSrc = (unsigned short *)(heightmapSrc.getData());
	const int nPixelX = (width - 1) / mapInfo.nGridX;
	const int nPixelZ = (height - 1) / mapInfo.nGridZ;

	// check all the raw data for LOD level 0
	HeightMinMax* h = new HeightMinMax[mapInfo.nGridX * mapInfo.nGridZ];
	_heightMinMax.push_back(h);
	for(size_t iz=0; iz<mapInfo.nGridZ; iz++)
	{
		for(size_t ix=0; ix<mapInfo.nGridX; ix++, h++)
		{
			unsigned short minY = 65535;
			unsigned short maxY = 0;
			for(int z=0; z<=nPixelZ; z++)
			{
				const unsigned short *pSrc = pImgSrc + (iz * nPixelZ + z) * width + ix * nPixelX;
				for(int x=0; x<=nPixelX; x++, pSrc++)
				{
					if (*pSrc < minY) minY = *pSrc;
					if (*pSrc > maxY) maxY = *pSrc;
				}
			}
			h->minY = minY;
			h->maxY = maxY;
		}
	}

	for(int lodLevel=1; lodLevel <_LODLevelCount; lodLevel++)
	{
		unsigned int divider = 1 << lodLevel;
		unsigned int nGridX = mapInfo.nGridX / divider;
		unsigned int nGridZ = mapInfo.nGridZ / divider;
		h = new HeightMinMax[nGridX * nGridZ];
		_heightMinMax.push_back(h);
		for(size_t iz=0; iz<nGridZ; iz++)
		{
			for(size_t ix=0; ix<nGridX; ix++, h++)
			{
				const HeightMinMax& lower0 = getHeightMinMax(lodLevel-1, ix*2,   iz*2);
				const HeightMinMax& lower1 = getHeightMinMax(lodLevel-1, ix*2+1, iz*2);
				const HeightMinMax& lower2 = getHeightMinMax(lodLevel-1, ix*2,   iz*2+1);
				const HeightMinMax& lower3 = getHeightMinMax(lodLevel-1, ix*2+1, iz*2+1);
				unsigned short minY = lower0.minY;
				unsigned short maxY = lower0.maxY;
				if (minY > lower1.minY) minY = lower1.minY;
				if (minY > lower2.minY) minY = lower2.minY;
				if (minY > lower3.minY) minY = lower3.minY;
				h->minY = minY;
				if (maxY < lower1.maxY) maxY = lower1.maxY;
				if (maxY < lower2.maxY) maxY = lower2.maxY;
				if (maxY < lower3.maxY) maxY = lower3.maxY;
				h->maxY = maxY;
			}
		}
	}
	_heightmap_width = width;
	_heightmap_height = height;
	//save_h();
}

Ogre::MaterialPtr& GetMaterial()
{
	return _material;
}

void OgreMaterialInit(const MapDimensions& map, int gridDimension, const char* heightmapName, const char* hmap2Name)
{
	Ogre::MaterialPtr baseMaterial = Ogre::MaterialManager::getSingleton().getByName(_baseMaterialName);
	_material = baseMaterial->clone(_baseMaterialName + Ogre::StringConverter::toString(_nMaterial++));
	Ogre::Pass* pass = _material->getTechnique(0)->getPass(0);
	Ogre::GpuProgramParametersSharedPtr vProgram = pass->getVertexProgramParameters();
	float f[4] = {map.MinX, map.MinZ, map.gridSizeX, map.gridSizeZ};
	vProgram->setNamedConstant("mapDimensions", f, 1);
	f[0] = (gridDimension-1) * 0.5f;
	f[1] = 1.0f / f[0];
	f[2] = map.MinY;
	f[3] = map.SizeY;
	vProgram->setNamedConstant("gridDim", f, 1);

	Ogre::GpuProgramParametersSharedPtr fProgram = pass->getFragmentProgramParameters();
	fProgram->setNamedConstant("gridDim", f, 1);
	f[0] = 1.0f/_heightmap_width;
	f[1] = 1.0f/_heightmap_height;
	f[2] = (_heightmap_height-1)/map.SizeX;
	f[3] = (_heightmap_width-1)/map.SizeZ;
	fProgram->setNamedConstant("texelSize", f, 1);

	pass->getTextureUnitState(0)->setTextureName(heightmapName);
	pass->getTextureUnitState(1)->setTextureName(hmap2Name);
	pass->getTextureUnitState(2)->setTextureName(heightmapName);
	pass->getTextureUnitState(3)->setTextureName(hmap2Name);
}

void OgreUpdateHeightmapBlendRatio(float ratio)
{
	for (size_t i = 0; i < _nMaterial; ++i)
	{
		Ogre::MaterialPtr matPtr = Ogre::MaterialManager::getSingleton().getByName(_baseMaterialName + Ogre::StringConverter::toString(i));
		Ogre::GpuProgramParametersSharedPtr vProgram = matPtr->getTechnique(0)->getPass(0)->getVertexProgramParameters();
		Ogre::GpuProgramParametersSharedPtr fProgram = matPtr->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
		if (!vProgram.isNull())
			vProgram->setNamedConstant("heightBlendRatio", ratio);
		if (!fProgram.isNull())
			fProgram->setNamedConstant("heightBlendRatio", ratio);
	}
}

void LOD_init(const MapDimensions& mapInfo, int lodLevelCount, int gridDim, float morphStartRatio, const char* heightmapName, const char* hmap2Name)
{
	float currentDetailBalance = 1.0f;

	_LODLevelCount = lodLevelCount;
	_morphStartRatio = morphStartRatio;

	_lodRangeDistRatios.reserve(lodLevelCount);
	_lodSqRanges.reserve(lodLevelCount);
	_lodSqRanges.resize(lodLevelCount);

	_morphConsts.reserve(lodLevelCount);
	_morphConsts.resize(lodLevelCount);

	_nMaxLODSize = 1 << (lodLevelCount-1);

	_mapInfo = mapInfo;

	// this is a hack to work around morphing problems with the first two LOD levels
	// (makes the zero LOD level a bit shorter)
	//_lodRangeDistRatios.push_back(currentDetailBalance * 0.9f);
	//currentDetailBalance *= _LODLevelDistanceRatio;

	for( int i = 0; i < lodLevelCount-1; i++ )
	{
		_lodRangeDistRatios.push_back(currentDetailBalance);
		currentDetailBalance *= _LODLevelDistanceRatio;
	}
	_lodRangeDistRatios.push_back(currentDetailBalance);
	for( int i = 0; i < lodLevelCount; i++ )
	{
		_lodRangeDistRatios[i] /= currentDetailBalance;
	}

	// do this for hmap1 only for now
	ConstructLODfromHeightmap(mapInfo, heightmapName);
	OgreMaterialInit(mapInfo, gridDim, heightmapName, hmap2Name);
}

void LOD_deinit()
{
	while(_heightMinMax.size())
	{
		delete [] _heightMinMax.back();
		_heightMinMax.pop_back();
	}

	if (!_material.isNull())
		_material.setNull();
}