#pragma once
#include "Proton/Scene/Entity.h"

namespace proton {

	class NetClientTransformSyncSystem
	{
	public:
		static void Update(Scene* scene, float ts);
		static void UpdatePhysics(Scene* scene, float ts);

	private:
		static float s_ExtrapolationTimeThreshold;
	};

}
