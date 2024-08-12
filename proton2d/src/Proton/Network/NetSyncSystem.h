#pragma once

namespace proton {

	class Scene;

	class NetSyncSystem
	{
	public:
		static void Update(Scene* scene, float ts);
		static void UpdatePhysics(Scene* scene, float ts);
	};

}
