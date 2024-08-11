#pragma once

namespace proton {

	class NetTransform
	{
        enum class ReplicationFlags
        {
            None = 0, // No sync

            PositionX = 1 << 0,
            PositionY = 1 << 1,
            PositionZ = 1 << 2,
            Position = PositionX | PositionY | PositionZ,

            ScaleX = 1 << 3,
            ScaleY = 1 << 4,
            ScaleZ = 1 << 5,
            Scale = ScaleX | ScaleY | ScaleZ,

            RotationX = 1 << 6,
            RotationY = 1 << 7,
            RotationZ = 1 << 8,
            Rotation = RotationX | RotationY | RotationZ,

            All = Position | Scale | Rotation,
        };

		struct Transform
		{
			glm::vec3 Position;
			glm::vec2 Scale;
			float Rotation;
		};

		struct Entry
		{
			Transform Value;
			uint16_t SequenceNumber;
		};


		Transform LastFrameTransform;

	};

}
