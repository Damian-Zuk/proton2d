#pragma once

namespace proton {

	// Supported script variable types for serialization / editor view.
	// `ScriptFieldType::Data` must be a trivial type (currently not serializable - `SceneSerializer`)
	enum class ScriptFieldType { Float, Float2, Float3, Float4, Int, Int2, Int3, Int4, Bool, String, Data };

	struct ScriptField
	{
		ScriptFieldType Type;
		void* ValuePtr = nullptr;
		size_t Size;
	};

#ifdef PT_EDITOR
	// Editor script field additional information
	struct EditorScriptField
	{
		bool ShowInEditor = true;
	};
#endif

}
