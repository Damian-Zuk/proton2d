#pragma once

namespace proton {

	struct InterpolationData  
	{
		glm::vec3 ServerPosition;
		glm::vec2 ServerLinearVelocity;

		float ServerRotation;
		float ServerAngularVelocity;

		float PacketDelay;
		Timer UpdateTimer;
	};

}
