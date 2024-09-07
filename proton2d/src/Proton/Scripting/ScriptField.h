#pragma once

namespace proton {

	// Supported script variable types for serialization / editor view.
	// ScriptFieldType::Data must be trivial type
	enum class ScriptFieldType { Float, Float2, Float3, Float4, Int, Int2, Int3, Int4, Bool, String, Data };

	struct ScriptField
	{
		ScriptFieldType Type = ScriptFieldType::Float;
		void* ValuePtr = nullptr;
		size_t Size = 0;
	};

#ifdef PT_EDITOR
	// Editor script field additional information
	struct EditorScriptField
	{
		bool ShowInEditor = true;
	};
#endif

}
