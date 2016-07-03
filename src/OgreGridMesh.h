#pragma once

namespace Ogre
{
	class SceneManager;
	class SceneNode;
}

class OgreGridMesh
{
private:
	int m_dimension;
	float m_gridsize;
	Ogre::SceneNode* m_node;

	void createMesh();
public:
	OgreGridMesh(Ogre::SceneManager* scnMgr, int dim, float size);
	~OgreGridMesh()
	{
		OnDestroyDevice();
	}

	void setDimension(int dim);
	int getDimension() { return m_dimension; }

	void setPosition(float x, float y, float z);

private:
	virtual int OnCreateDevice();
	virtual void OnDestroyDevice();
};