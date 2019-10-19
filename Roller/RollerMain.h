#pragma once

#include "Common\StepTimer.h"
#include "Common\AlteredDevice.h"
#include "Content\Hvy3DScene.h"
#include "Content\HUD.h"



namespace Roller
{
	class RollerMain : public DX::IDeviceNotify
	{
	public:
		RollerMain(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~RollerMain();
		void CreateWindowSizeDependentResources();
		void Update();
		bool Render();

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;


		std::unique_ptr<HvyDX::Hvy3DScene> m_sceneRenderer;

		std::unique_ptr<HUD> m_fpsTextRenderer;


		// Rendering loop timer.
		DX::StepTimer m_timer;
	};
}
