#pragma once
#include "Proton/Scene/Entity.h"

namespace proton {

	class NetInterpolationSystem
	{
	public:
		void Interpolate(Scene* scene, float ts);
		void Extrapolate(Scene* scene, float ts);
		void OnUpdate(Scene* scene, float ts);

	private:
	};

}
