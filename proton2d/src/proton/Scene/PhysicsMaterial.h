#pragma once

namespace proton {

	struct PhysicsMaterial
	{
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;
		float Density = 0.5f;
	};

}