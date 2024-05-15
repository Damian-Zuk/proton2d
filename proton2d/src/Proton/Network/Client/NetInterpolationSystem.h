#pragma once
#include "Proton/Scene/Entity.h"

namespace proton {

	class NetInterpolationSystem
	{
	public:
		static void InterpolateAll(Scene* scene, float ts);
	};

}
