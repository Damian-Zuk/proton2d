#pragma once

namespace proton {

	// Supported script variable types for serialization / editor view.
	// `ScriptFieldType::Data` must be a trivial type (currently not serializable using SceneSerializer)
	enum class ScriptFieldType 
	{
		Float, Float2, Float3, Float4, Int, Int2, Int3, Int4, Bool, String, Data 
	};

	inline size_t GetScriptFieldSize(ScriptFieldType type)
	{
		switch (type)
		{
		case ScriptFieldType::Float:  return sizeof(float);
		case ScriptFieldType::Float2: return sizeof(glm::vec2);
		case ScriptFieldType::Float3: return sizeof(glm::vec3);
		case ScriptFieldType::Float4: return sizeof(glm::vec4);
		case ScriptFieldType::Int:    return sizeof(int);
		case ScriptFieldType::Int2:   return sizeof(glm::ivec2);
		case ScriptFieldType::Int3:   return sizeof(glm::ivec3);
		case ScriptFieldType::Int4:   return sizeof(glm::ivec4);
		case ScriptFieldType::Bool:   return sizeof(bool);
		case ScriptFieldType::String: return sizeof(std::string);
		}
		return 4;
	}

	struct ScriptField
	{
		void* ValuePtr = nullptr;
		ScriptFieldType Type;
		size_t Size = 0;
		bool ShowInEditor = true;
	};

#ifdef PT_EDITOR
	// Editor script field additional information
	struct EditorScriptField
	{
		bool ShowInEditor = true;
	};
#endif

}
