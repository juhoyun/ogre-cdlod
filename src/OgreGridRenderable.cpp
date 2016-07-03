#include "OgreGridRenderable.h"
#include "OgreHardwareBufferManager.h"
#include "OgreMaterialManager.h"
#include "OgreTechnique.h"
#include "OgreCamera.h"

void LOD_getMorphConsts(int lodLevel, float consts[]);
const MapDimensions& LOD_getMapInfo();
Ogre::MaterialPtr& GetMaterial();

namespace Ogre
{
	VertexData* OgreGridRenderable::vertexData;
	IndexData* OgreGridRenderable::indexData[16];
	LightList OgreGridRenderable::lightList;

	inline float getWorldHeight(float y)
	{
		const MapDimensions& mapInfo = LOD_getMapInfo();
		return y * mapInfo.SizeY / 65535.0f + mapInfo.MinY;
	}

	const MaterialPtr& OgreGridRenderable::getMaterial(void) const
	{
		return GetMaterial();
	}

	void OgreGridRenderable::initOgreGridRenderable(int gridDimension)
	{
		// TODO: considering multiple/dynamic initialization,
		// maybe need to free the existing vertex/index buffer

		vertexData = new VertexData();
		vertexData->vertexCount = gridDimension * gridDimension;

		// vertex declaration
		VertexDeclaration* decl = vertexData->vertexDeclaration;
		size_t offset = 0;
		decl->addElement(0, offset, VET_FLOAT3, VES_POSITION);
		offset += VertexElement::getTypeSize(VET_FLOAT3);
		decl->addElement(0, offset, VET_FLOAT3, VES_NORMAL);
		offset += VertexElement::getTypeSize(VET_FLOAT3);

		HardwareBufferManager& hbm = HardwareBufferManager::getSingleton();
		HardwareVertexBufferSharedPtr vbuf = hbm.createVertexBuffer(
				offset, vertexData->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY);
		VertexBufferBinding* bind = vertexData->vertexBufferBinding;
		bind->setBinding(0, vbuf);
		float* pVertex = static_cast<float*>(vbuf->lock(HardwareBuffer::HBL_DISCARD));
		float scale = 1.0f / (gridDimension - 1);
		for (int z = 0; z < gridDimension; z++)
		{
			float zPos = z * scale;
			for (int x = 0; x < gridDimension; x++)
			{
				// Position
				*pVertex++ = x * scale;
				*pVertex++ = 0;
				*pVertex++ = zPos;
				// Normal
				*pVertex++ = 0;
				*pVertex++ = 1;
				*pVertex++ = 0;
			}
		}
		vbuf->unlock();

		const int ibufCount = (gridDimension - 1) * (gridDimension - 1) * 6;
		unsigned short* indexBuffer = new unsigned short[ibufCount];
		unsigned short* pIndices = indexBuffer;

	#define ICOORD(x,z)	((x) + (z)*gridDimension)
#if 0
		for (int z = 0; z < gridDimension - 1; z++)
		{
			for (int x = 0; x < gridDimension - 1; x++)
			{
				*pIndices++ = ICOORD(x, z);
				*pIndices++ = ICOORD(x, z + 1);
				*pIndices++ = ICOORD(x + 1, z);

				*pIndices++ = ICOORD(x + 1, z);
				*pIndices++ = ICOORD(x, z + 1);
				*pIndices++ = ICOORD(x + 1, z + 1);
			}
		}
#else
		int halfDim = (gridDimension - 1) / 2;
		int index = 0;
		int indexEndTL;
		int indexEndTR;
		int indexEndBL;
		int indexEndBR;
		// Top Left
		for (int z = 0; z < halfDim; z++)
		{
			for (int x = 0; x < halfDim; x++, index += 6)
			{
				*pIndices++ = ICOORD(x, z);
				*pIndices++ = ICOORD(x, z + 1);
				*pIndices++ = ICOORD(x + 1, z);

				*pIndices++ = ICOORD(x + 1, z);
				*pIndices++ = ICOORD(x, z + 1);
				*pIndices++ = ICOORD(x + 1, z + 1);
			}
		}
		int childNodeIndexCount = indexEndTL = index;
		// Top Right
		for (int z = 0; z < halfDim; z++)
		{
			for (int x = halfDim; x < gridDimension - 1; x++, index += 6)
			{
				*pIndices++ = ICOORD(x, z);
				*pIndices++ = ICOORD(x, z + 1);
				*pIndices++ = ICOORD(x + 1, z);

				*pIndices++ = ICOORD(x + 1, z);
				*pIndices++ = ICOORD(x, z + 1);
				*pIndices++ = ICOORD(x + 1, z + 1);
			}
		}
		indexEndTR = index;
		// Bottom Left
		for (int z = halfDim; z < gridDimension - 1; z++)
		{
			for (int x = 0; x < halfDim; x++, index += 6)
			{
				*pIndices++ = ICOORD(x, z);
				*pIndices++ = ICOORD(x, z + 1);
				*pIndices++ = ICOORD(x + 1, z);

				*pIndices++ = ICOORD(x + 1, z);
				*pIndices++ = ICOORD(x, z + 1);
				*pIndices++ = ICOORD(x + 1, z + 1);
			}
		}
		indexEndBL = index;
		// Bottom Right
		for (int z = halfDim; z < gridDimension - 1; z++)
		{
			for (int x = halfDim; x < gridDimension - 1; x++, index += 6)
			{
				*pIndices++ = ICOORD(x, z);
				*pIndices++ = ICOORD(x, z + 1);
				*pIndices++ = ICOORD(x + 1, z);

				*pIndices++ = ICOORD(x + 1, z);
				*pIndices++ = ICOORD(x, z + 1);
				*pIndices++ = ICOORD(x + 1, z + 1);
			}
		}
		indexEndBR = index;
#endif
		const int sizeUS = sizeof(unsigned short);
		const size_t sizeInBytes = childNodeIndexCount * sizeUS;
		const HardwareBuffer::Usage hbu = HardwareBuffer::HBU_STATIC_WRITE_ONLY;
		const HardwareIndexBuffer::IndexType itype = HardwareIndexBuffer::IT_16BIT;
		HardwareIndexBufferSharedPtr ibuf[5];

		// All
		ibuf[0] = hbm.createIndexBuffer(itype, ibufCount, hbu);
		ibuf[0]->writeData(0, ibufCount * sizeUS, indexBuffer);

		// TL, BL
		ibuf[1] = hbm.createIndexBuffer(itype, childNodeIndexCount * 2, hbu);
		// copy TL
		ibuf[1]->writeData(0, sizeInBytes, indexBuffer);
		// copy BL
		ibuf[1]->writeData(sizeInBytes, sizeInBytes, indexBuffer + indexEndTR);

		// TR, BR
		ibuf[2] = hbm.createIndexBuffer(itype, childNodeIndexCount * 2, hbu);
		// copy TR
		ibuf[2]->writeData(0, sizeInBytes, indexBuffer + indexEndTL);
		// copy BR
		ibuf[2]->writeData(sizeInBytes, sizeInBytes, indexBuffer + indexEndBL);

		// TL, TR, BR
		ibuf[3] = hbm.createIndexBuffer(itype, childNodeIndexCount * 3, hbu);
		// copy TL, TR
		ibuf[3]->writeData(0, 2 * sizeInBytes, indexBuffer);
		// copy BR
		ibuf[3]->writeData(2 * sizeInBytes, sizeInBytes, indexBuffer + indexEndBL);

		// TL, BL, BR
		ibuf[4] = hbm.createIndexBuffer(itype, childNodeIndexCount * 3, hbu);
		// copy TL
		ibuf[4]->writeData(0, sizeInBytes, indexBuffer);
		// copy BL, BR
		ibuf[4]->writeData(sizeInBytes, 2 * sizeInBytes, indexBuffer + indexEndTR);

		delete[] indexBuffer;

		for (int i = 0; i < 16; i++)
			indexData[i] = new IndexData();

		// None
		indexData[0]->indexBuffer = ibuf[0];
		indexData[0]->indexStart = 0;
		indexData[0]->indexCount = ibufCount;

		// TL
		indexData[1]->indexBuffer = ibuf[0];
		indexData[1]->indexStart = 0;
		indexData[1]->indexCount = childNodeIndexCount;

		// TR
		indexData[2]->indexBuffer = ibuf[0];
		indexData[2]->indexStart = indexEndTL;
		indexData[2]->indexCount = childNodeIndexCount;

		// TL, TR
		indexData[3]->indexBuffer = ibuf[0];
		indexData[3]->indexStart = 0;
		indexData[3]->indexCount = childNodeIndexCount * 2;

		// BL
		indexData[4]->indexBuffer = ibuf[0];
		indexData[4]->indexStart = indexEndTR;
		indexData[4]->indexCount = childNodeIndexCount;

		// TL, BL
		indexData[5]->indexBuffer = ibuf[1];
		indexData[5]->indexStart = 0;
		indexData[5]->indexCount = childNodeIndexCount * 2;

		// TR, BL
		indexData[6]->indexBuffer = ibuf[0];
		indexData[6]->indexStart = indexEndTL;
		indexData[6]->indexCount = childNodeIndexCount * 2;

		// TL, TR, BL
		indexData[7]->indexBuffer = ibuf[0];
		indexData[7]->indexStart = 0;
		indexData[7]->indexCount = childNodeIndexCount * 3;

		// BR
		indexData[8]->indexBuffer = ibuf[0];
		indexData[8]->indexStart = indexEndBL;
		indexData[8]->indexCount = childNodeIndexCount;

		// TL, BR - possible?
		indexData[9]->indexBuffer = ibuf[2];
		indexData[9]->indexStart = 0;
		indexData[9]->indexCount = childNodeIndexCount * 2;

		// TR, BR
		indexData[10]->indexBuffer = ibuf[2];
		indexData[10]->indexStart = 0;
		indexData[10]->indexCount = childNodeIndexCount * 2;

		// TL, TR, BR
		indexData[11]->indexBuffer = ibuf[3];
		indexData[11]->indexStart = 0;
		indexData[11]->indexCount = childNodeIndexCount * 3;
		
		// BL, BR
		indexData[12]->indexBuffer = ibuf[0];
		indexData[12]->indexStart = indexEndTR;
		indexData[12]->indexCount = childNodeIndexCount * 2;

		// TL, BL, BR
		indexData[13]->indexBuffer = ibuf[4];
		indexData[13]->indexStart = 0;
		indexData[13]->indexCount = childNodeIndexCount * 3;

		// TR, BL, BR
		indexData[14]->indexBuffer = ibuf[0];
		indexData[14]->indexStart = indexEndTL;
		indexData[14]->indexCount = childNodeIndexCount * 3;

		// All
		indexData[15]->indexBuffer = ibuf[0];
		indexData[15]->indexStart = 0;
		indexData[15]->indexCount = ibufCount;
	}

	void OgreGridRenderable::deinitOgreGridRenderable()
	{
	}

	void OgreGridRenderable::_updateCustomGpuParameter(
            const GpuProgramParameters::AutoConstantEntry& constantEntry,
            GpuProgramParameters* params) const
	{
		float f[4];
		switch(constantEntry.data)
		{
		case 10: // nodeInfo
			f[0] = (float)nodeInfo.X;
			f[1] = (float)nodeInfo.Z;
			f[2] = (float)nodeInfo.Size;
			f[3] = (float)nodeInfo.LODLevel;	// for test
			params->_writeRawConstants(constantEntry.physicalIndex, f, 4);
			break;
		case 11: // morphConsts
			LOD_getMorphConsts(nodeInfo.LODLevel, f);
			params->_writeRawConstants(constantEntry.physicalIndex, f, 4);
			break;
		case 12: // nodeCoeff
		{
			const MapDimensions& mapInfo = LOD_getMapInfo();
			f[0] = (float)nodeInfo.X / mapInfo.nGridX;
			f[1] = (float)nodeInfo.Z / mapInfo.nGridZ;
			f[2] = (float)nodeInfo.Size / mapInfo.nGridX;
			f[3] = (float)nodeInfo.Size / mapInfo.nGridZ;
			params->_writeRawConstants(constantEntry.physicalIndex, f, 4);
			break;
		}
		default:
			Renderable::_updateCustomGpuParameter(constantEntry, params);
		}
	}

	void OgreGridRenderable::getRenderOperation(RenderOperation& op)
	{
		int i = 0;
		if (nodeInfo.TL) i |= 1;
		if (nodeInfo.TR) i |= 2;
		if (nodeInfo.BL) i |= 4;
		if (nodeInfo.BR) i |= 8;

		op.useIndexes = true;
		op.operationType = RenderOperation::OT_TRIANGLE_LIST;
		op.vertexData = vertexData;
		op.indexData = indexData[i];
	}

	Real OgreGridRenderable::getSquaredViewDepth(const Camera* cam) const
	{
		const MapDimensions& mapInfo = LOD_getMapInfo();
		Vector3 center(
			(nodeInfo.X + nodeInfo.Size * 0.5f) * mapInfo.gridSizeX + mapInfo.MinX,
			getWorldHeight(0.5f * (nodeInfo.MinY + nodeInfo.MaxY)),
			(nodeInfo.Z + nodeInfo.Size * 0.5f) * mapInfo.gridSizeZ + mapInfo.MinZ);
		Vector3 diff = center - cam->getDerivedPosition();
		return diff.squaredLength();
	}

	const String& OgreGridRenderable::getMovableType(void) const
	{
		static String movType = "OgreGridRenderable";
        return movType;
	}

	const AxisAlignedBox& OgreGridRenderable::getBoundingBox(void) const
	{
		return aabb;
	}

	Real OgreGridRenderable::getBoundingRadius(void) const
	{
		return nodeInfo.Size * LOD_getMapInfo().gridSizeX * 0.5f;
	}

	void OgreGridRenderable::_updateRenderQueue(RenderQueue* queue)
	{
		queue->addRenderable(this, RENDER_QUEUE_WORLD_GEOMETRY_1);
	}

	void OgreGridRenderable::visitRenderables(Renderable::Visitor* visitor, bool debugRenderables)
	{
		visitor->visit(this, 0, false);
	}

	void OgreGridRenderable::setNodeInfo(const NodeInfo& ni)
	{
		nodeInfo = ni;
		const MapDimensions& mapInfo = LOD_getMapInfo();
		aabb.setMinimum(
			ni.X * mapInfo.gridSizeX + mapInfo.MinX,
			getWorldHeight(ni.MinY),
			ni.Z * mapInfo.gridSizeZ + mapInfo.MinZ);
		aabb.setMaximum(
			(ni.X + ni.Size) * mapInfo.gridSizeX + mapInfo.MinX,
			getWorldHeight(ni.MaxY),
			(ni.Z + ni.Size) * mapInfo.gridSizeZ + mapInfo.MinZ);
	}
}