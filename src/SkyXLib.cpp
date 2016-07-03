#include "SkyX.h"


/** SkyX settings struct
@remarks These just are the most important SkyX parameters, not all SkyX parameters.
*/
struct SkyXSettings
{
	/** Constructor
	@remarks Skydome + vol. clouds + lightning settings
	*/
	SkyXSettings(const Ogre::Vector3 t, const Ogre::Real& tm, const Ogre::Real& mp, const SkyX::AtmosphereManager::Options& atmOpt,
	const bool& lc, const bool& vc, const Ogre::Real& vcws, const bool& vcauto, const Ogre::Radian& vcwd,
	const Ogre::Vector3& vcac, const Ogre::Vector4& vclr, const Ogre::Vector4& vcaf, const Ogre::Vector2& vcw,
	const bool& vcl, const Ogre::Real& vclat, const Ogre::Vector3& vclc, const Ogre::Real& vcltm)
	: time(t), timeMultiplier(tm), moonPhase(mp), atmosphereOpt(atmOpt), layeredClouds(lc), volumetricClouds(vc), vcWindSpeed(vcws)
	, vcAutoupdate(vcauto), vcWindDir(vcwd), vcAmbientColor(vcac), vcLightResponse(vclr), vcAmbientFactors(vcaf), vcWheater(vcw)
	, vcLightnings(vcl), vcLightningsAT(vclat), vcLightningsColor(vclc), vcLightningsTM(vcltm)
	{}

	/** Constructor
	@remarks Skydome + vol. clouds
	*/
	SkyXSettings(const Ogre::Vector3 t, const Ogre::Real& tm, const Ogre::Real& mp, const SkyX::AtmosphereManager::Options& atmOpt,
		const bool& lc, const bool& vc, const Ogre::Real& vcws, const bool& vcauto, const Ogre::Radian& vcwd,
		const Ogre::Vector3& vcac, const Ogre::Vector4& vclr, const Ogre::Vector4& vcaf, const Ogre::Vector2& vcw)
		: time(t), timeMultiplier(tm), moonPhase(mp), atmosphereOpt(atmOpt), layeredClouds(lc), volumetricClouds(vc), vcWindSpeed(vcws)
		, vcAutoupdate(vcauto), vcWindDir(vcwd), vcAmbientColor(vcac), vcLightResponse(vclr), vcAmbientFactors(vcaf), vcWheater(vcw), vcLightnings(false)
	{}

	/** Constructor
	@remarks Skydome settings
	*/
	SkyXSettings(const Ogre::Vector3 t, const Ogre::Real& tm, const Ogre::Real& mp, const SkyX::AtmosphereManager::Options& atmOpt, const bool& lc)
		: time(t), timeMultiplier(tm), moonPhase(mp), atmosphereOpt(atmOpt), layeredClouds(lc), volumetricClouds(false), vcLightnings(false)
	{}

	/// Time
	Ogre::Vector3 time;
	/// Time multiplier
	Ogre::Real timeMultiplier;
	/// Moon phase
	Ogre::Real moonPhase;
	/// Atmosphere options
	SkyX::AtmosphereManager::Options atmosphereOpt;
	/// Layered clouds?
	bool layeredClouds;
	/// Volumetric clouds?
	bool volumetricClouds;
	/// VClouds wind speed
	Ogre::Real vcWindSpeed;
	/// VClouds autoupdate
	bool vcAutoupdate;
	/// VClouds wind direction
	Ogre::Radian vcWindDir;
	/// VClouds ambient color
	Ogre::Vector3 vcAmbientColor;
	/// VClouds light response
	Ogre::Vector4 vcLightResponse;
	/// VClouds ambient factors
	Ogre::Vector4 vcAmbientFactors;
	/// VClouds wheater
	Ogre::Vector2 vcWheater;
	/// VClouds lightnings?
	bool vcLightnings;
	/// VClouds lightnings average aparition time
	Ogre::Real vcLightningsAT;
	/// VClouds lightnings color
	Ogre::Vector3 vcLightningsColor;
	/// VClouds lightnings time multiplier
	Ogre::Real vcLightningsTM;
};

/** Demo presets
@remarks The best way of determinate each parameter value is by using a real-time editor.
These presets have been quickly designed using the Paradise Editor, which is a commercial solution.
At the time I'm writting these lines, SkyX 0.1 is supported by Ogitor. Hope that the Ogitor team will
support soon SkyX 0.4, this way you all are going to be able to quickly create cool SkyX configurations.
*/
static SkyXSettings mPresets[] = {
	// Sunset
	SkyXSettings(Ogre::Vector3(8.85f,  7.5f, 20.5f), -0.08f, 0, SkyX::AtmosphereManager::Options(9.77501f, 10.2963f, 0.01f, 0.0022f, 0.000675f, 30, Ogre::Vector3(0.57f, 0.52f, 0.44f), -0.991f, 3, 4), false, true, 300, false, Ogre::Radian(270), Ogre::Vector3(0.63f, 0.63f, 0.7f), Ogre::Vector4(0.35f, 0.2f, 0.92f, 0.1f), Ogre::Vector4(0.4f, 0.7f, 0, 0), Ogre::Vector2(0.8f, 1)),
	// Clear
	SkyXSettings(Ogre::Vector3(17.16f, 7.5f, 20.5f), 0, 0, SkyX::AtmosphereManager::Options(9.77501f, 10.2963f, 0.01f, 0.0017f, 0.000675f, 30, Ogre::Vector3(0.57f, 0.54f, 0.44f), -0.991f, 2.5f, 4), false),
	// Thunderstorm 1
	SkyXSettings(Ogre::Vector3(12.23f, 7.5f, 20.5f), 0, 0, SkyX::AtmosphereManager::Options(9.77501f, 10.2963f, 0.01f, 0.00545f, 0.000375f, 30, Ogre::Vector3(0.55f, 0.54f, 0.52f), -0.991f, 1, 4), false, true, 300, false, Ogre::Radian(0), Ogre::Vector3(0.63f, 0.63f, 0.7f), Ogre::Vector4(0.25f, 0.4f, 0.5f, 0.1f), Ogre::Vector4(0.45f, 0.3f, 0.6f, 0.1f), Ogre::Vector2(1, 1), true, 0.5f, Ogre::Vector3(1, 0.976f, 0.92f), 2),
	// Thunderstorm 2
	SkyXSettings(Ogre::Vector3(10.23f, 7.5f, 20.5f), 0, 0, SkyX::AtmosphereManager::Options(9.77501f, 10.2963f, 0.01f, 0.00545f, 0.000375f, 30, Ogre::Vector3(0.55f, 0.54f, 0.52f), -0.991f, 0.5, 4), false, true, 300, false, Ogre::Radian(0), Ogre::Vector3(0.63f, 0.63f, 0.7f), Ogre::Vector4(0, 0.02f, 0.34f, 0.24f), Ogre::Vector4(0.29f, 0.3f, 0.6f, 1), Ogre::Vector2(1, 1), true, 0.5f, Ogre::Vector3(0.95f, 1, 1), 2),
	// Desert
	SkyXSettings(Ogre::Vector3(7.59f,  7.5f, 20.5f), 0, -0.8f, SkyX::AtmosphereManager::Options(9.77501f, 10.2963f, 0.01f, 0.0072f, 0.000925f, 30, Ogre::Vector3(0.71f, 0.59f, 0.53f), -0.997f, 2.5f, 1), true),
	// Night
	SkyXSettings(Ogre::Vector3(21.5f,  7.5f, 20.5f), 0.03f, -0.25f, SkyX::AtmosphereManager::Options(), true)
};


void setSkyXPreset(int presetNo, SkyX::SkyX* skyX, SkyX::BasicController* controller, Ogre::Camera* camera)
{
	const SkyXSettings& preset = mPresets[presetNo];
	skyX->setTimeMultiplier(preset.timeMultiplier);
	controller->setTime(preset.time);
	controller->setMoonPhase(preset.moonPhase);
	skyX->getAtmosphereManager()->setOptions(preset.atmosphereOpt);

	// Layered clouds
	if (preset.layeredClouds)
	{
		// Create layer cloud
		if (skyX->getCloudsManager()->getCloudLayers().empty())
		{
			skyX->getCloudsManager()->add(SkyX::CloudLayer::Options(/* Default options */));
		}
	}
	else
	{
		// Remove layer cloud
		if (!skyX->getCloudsManager()->getCloudLayers().empty())
		{
			skyX->getCloudsManager()->removeAll();
		}
	}

	skyX->getVCloudsManager()->setWindSpeed(preset.vcWindSpeed);
	skyX->getVCloudsManager()->setAutoupdate(preset.vcAutoupdate);

	SkyX::VClouds::VClouds* vclouds = skyX->getVCloudsManager()->getVClouds();

	vclouds->setWindDirection(preset.vcWindDir);
	vclouds->setAmbientColor(preset.vcAmbientColor);
	vclouds->setLightResponse(preset.vcLightResponse);
	vclouds->setAmbientFactors(preset.vcAmbientFactors);
	vclouds->setWheater(preset.vcWheater.x, preset.vcWheater.y, false);

	if (preset.volumetricClouds)
	{
		// Create VClouds
		if (!skyX->getVCloudsManager()->isCreated())
		{
			// SkyX::MeshManager::getSkydomeRadius(...) works for both finite and infinite(=0) camera far clip distances
			skyX->getVCloudsManager()->create(skyX->getMeshManager()->getSkydomeRadius(camera));
		}
	}
	else
	{
		// Remove VClouds
		if (skyX->getVCloudsManager()->isCreated())
		{
			skyX->getVCloudsManager()->remove();
		}
	}

	vclouds->getLightningManager()->setEnabled(preset.vcLightnings);
	vclouds->getLightningManager()->setAverageLightningApparitionTime(preset.vcLightningsAT);
	vclouds->getLightningManager()->setLightningColor(preset.vcLightningsColor);
	vclouds->getLightningManager()->setLightningTimeMultiplier(preset.vcLightningsTM);

	skyX->update(0);
}

void getSkyXAtmosphereOptinos(int index, SkyX::AtmosphereManager::Options* options)
{
	*options = mPresets[index].atmosphereOpt;
}