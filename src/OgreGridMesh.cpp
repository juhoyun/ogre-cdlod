#include "OgreSceneManager.h"
#include "OgreEntity.h"
#include "OgreMesh.h"
#include "OgreSubMesh.h"
#include "OgreMeshManager.h"
#include "OgreStringConverter.h"
#include "OgreHardwareBufferManager.h"
#include "OgreString.h"

#include "OgreGridMesh.h"

using namespace Ogre;

static bool _mesh_created = false;
static const char* _mesh_name = "OgreGridMesh";
static const char* _material_name = "OgreGridMeshMaterial";

void OgreGridMesh::createMesh()
{
	MeshPtr mesh = MeshManager::getSingleton().createManual(_mesh_name, "General");
	SubMesh* sub = mesh->createSubMesh();
	mesh->sharedVertexData = new VertexData();
	mesh->sharedVertexData->vertexCount = m_dimension * m_dimension;

	// vertex declaration
	VertexDeclaration* decl = mesh->sharedVertexData->vertexDeclaration;
	size_t offset = 0;
	decl->addElement(0, offset, VET_FLOAT3, VES_POSITION);
	offset += VertexElement::getTypeSize(VET_FLOAT3);
	decl->addElement(0, offset, VET_FLOAT3, VES_NORMAL);
	offset += VertexElement::getTypeSize(VET_FLOAT3);

	HardwareVertexBufferSharedPtr vbuf =
		HardwareBufferManager::getSingleton().createVertexBuffer(
		offset, mesh->sharedVertexData->vertexCount,
		HardwareBuffer::HBU_STATIC_WRITE_ONLY);
	VertexBufferBinding* bind = mesh->sharedVertexData->vertexBufferBinding;
	bind->setBinding(0, vbuf);
	float* pVertex = static_cast<float*>(vbuf->lock(HardwareBuffer::HBL_DISCARD));
	float scale = 1.0f / (m_dimension - 1);
	for (int z = 0; z < m_dimension; z++)
	{
		//float zPos = 2.0f * z / (m_dimension - 1) - 1.0f;
		float zPos = z * scale;
		for (int x = 0; x < m_dimension; x++)
		{
			// Position
			//*pVertex++ = 2.0f * x / (m_dimension - 1) - 1.0f;
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

	const int ibufCount = (m_dimension - 1) * (m_dimension - 1) * 6;
	HardwareIndexBufferSharedPtr ibuf =
		HardwareBufferManager::getSingleton().createIndexBuffer(
		HardwareIndexBuffer::IT_16BIT,
		ibufCount,
		HardwareBuffer::HBU_STATIC_WRITE_ONLY);
	unsigned short* pIndices = static_cast<unsigned short*>(ibuf->lock(HardwareBuffer::HBL_DISCARD));

#define ICOORD(x,z)	((x) + (z)*m_dimension)

	for (int z = 0; z < m_dimension - 1; z++)
	{
		for (int x = 0; x < m_dimension - 1; x++)
		{
			*pIndices++ = ICOORD(x, z);
			*pIndices++ = ICOORD(x, z + 1);
			*pIndices++ = ICOORD(x + 1, z);

			*pIndices++ = ICOORD(x + 1, z);
			*pIndices++ = ICOORD(x, z + 1);
			*pIndices++ = ICOORD(x + 1, z + 1);
		}
	}
	ibuf->unlock();

	sub->useSharedVertices = true;
	sub->indexData->indexBuffer = ibuf;
	sub->indexData->indexCount = ibufCount;
	sub->indexData->indexStart = 0;

	mesh->_setBounds(AxisAlignedBox(-1, 0, -1, 1, 0, 1));
	mesh->_setBoundingSphereRadius(Math::Sqrt(2.0f));

}
int OgreGridMesh::OnCreateDevice()
{
	if (!_mesh_created)
		createMesh();
	return 0;
}

void OgreGridMesh::OnDestroyDevice()
{
	// TBD
}

void OgreGridMesh::setDimension(int dim)
{
	m_dimension = dim;

	OnDestroyDevice();
	OnCreateDevice();
}

OgreGridMesh::OgreGridMesh(Ogre::SceneManager* scnMgr, int dim, float size) : m_gridsize(size)
{
	static int entity_cnt = 0;
	setDimension(dim);
	Ogre::String entName(_mesh_name);
	Ogre::Entity *ent = scnMgr->createEntity(entName+"Ent"+Ogre::StringConverter::toString(entity_cnt++), _mesh_name);
	ent->setMaterialName(_material_name);
	m_node = scnMgr->getRootSceneNode()->createChildSceneNode();
	m_node->attachObject(ent);
	m_node->setScale(size, 1, size);
}

void OgreGridMesh::setPosition(float x, float y, float z)
{
	m_node->setPosition(x, y, z);
}