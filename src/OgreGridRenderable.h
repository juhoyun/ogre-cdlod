#include "OgreRenderable.h"
#include "OgreMovableObject.h"
#include "OgreAxisAlignedBox.h"

struct MapDimensions
{
	float	MinX;
	float	MinY;
	float	MinZ;
	float	SizeX;
	float	SizeY;
	float	SizeZ;
	// number of unit grids
	unsigned int nGridX;
	unsigned int nGridZ;
	// unit grid size in world space
	float gridSizeX;
	float gridSizeZ;

	float	MaxX() const   { return MinX + SizeX; }
	float	MaxY() const   { return MinY + SizeY; }
	float	MaxZ() const   { return MinZ + SizeZ; }
};

struct NodeInfo
{
	unsigned int   X;
	unsigned int   Z;
	unsigned short Size;
	unsigned short MinY;
	unsigned short MaxY;

	// these can be flags and can be combined into LODLevel
	bool           TL;
	bool           TR;
	bool           BL;
	bool           BR;
	int            LODLevel;

	NodeInfo()      {}
	NodeInfo( unsigned int x, unsigned int z, unsigned short size, unsigned short minY, unsigned short maxY, int LODLevel, bool tl, bool tr, bool bl, bool br )
		: X(x), Z(z), Size((unsigned short)size), MinY(minY), MaxY(maxY), LODLevel(LODLevel), TL(tl), TR(tr), BL(bl), BR(br)
	{}
};

namespace Ogre
{
	class Camera;

	class OgreGridRenderable : public Renderable, public MovableObject
	{
	private:
		NodeInfo nodeInfo;
		AxisAlignedBox aabb;

		static VertexData* vertexData;
		static IndexData* indexData[16];
		static LightList lightList;

	public:
		// pure virtual functions of Renderable
		virtual const MaterialPtr& getMaterial(void) const;
		virtual void getRenderOperation(RenderOperation& op);
		virtual void getWorldTransforms(Matrix4* xform) const
		{
			*xform = Matrix4::IDENTITY;
		}

		virtual Real getSquaredViewDepth(const Camera* cam) const;

		virtual const LightList& getLights(void) const
		{
			return lightList;
		}

		virtual void _updateCustomGpuParameter(
			const GpuProgramParameters::AutoConstantEntry& constantEntry,
			GpuProgramParameters* params) const;

		// pure virtual functions of MovableObject
		virtual const String& getMovableType(void) const;
		virtual const AxisAlignedBox& getBoundingBox(void) const;
		virtual Real getBoundingRadius(void) const;
		virtual void _updateRenderQueue(RenderQueue* queue);
		void visitRenderables(Renderable::Visitor* visitor, bool debugRenderables);

		// member functions

		void setName(const String& name) { mName = name; }

		static void addLight(Light* light)
		{
			lightList.push_back(light);
		}

		NodeInfo& getNodeInfo() { return nodeInfo; }
		void setNodeInfo(const NodeInfo& ni);

		static void initOgreGridRenderable(int gridDimension);
		static void deinitOgreGridRenderable();
	};
}