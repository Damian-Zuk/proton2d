#include "ptpch.h"
#include "Proton//Scripting/ScriptSerializer.h"
#include "Proton/Scripting/Script.h"


namespace proton {

	json ScriptSerializer::SerializeScript(Script* script)
	{
		json j;
		j["ClassName"] = script->GetScriptClassName();

		for (auto& [fieldName, fieldData] : script->m_ScriptFields)
		{
			json& fieldJson = j["Fields"][fieldName];
			void* valuePtr = fieldData.ValuePtr;

			#define WRITE_FIELD_DATA(EnumType, Type) \
				case ScriptFieldType::EnumType: fieldJson = *(Type*)valuePtr; break

			switch (fieldData.Type)
			{
				WRITE_FIELD_DATA(Float, float);
				WRITE_FIELD_DATA(Float2, glm::vec2);
				WRITE_FIELD_DATA(Float3, glm::vec3);
				WRITE_FIELD_DATA(Float4, glm::vec4);
				WRITE_FIELD_DATA(Int, int);
				WRITE_FIELD_DATA(Int2, glm::ivec2);
				WRITE_FIELD_DATA(Int3, glm::ivec3);
				WRITE_FIELD_DATA(Int4, glm::ivec4);
				WRITE_FIELD_DATA(Bool, bool);
				WRITE_FIELD_DATA(String, std::string);
			}
		}
		return j;
	}

	void ScriptSerializer::DeserializeScript(Script* script, const json& j)
	{
		//std::string className = j["ClassName"];

		if (!j.contains("Fields"))
			return;

		for (auto& [fieldName, fieldValue] : j["Fields"].items())
		{
			if (script->m_ScriptFields.find(fieldName) == script->m_ScriptFields.end())
				continue;

			const ScriptField& scriptField = script->m_ScriptFields[fieldName];
			void* valuePtr = scriptField.ValuePtr;

			#define READ_FIELD_DATA(EnumType, Type) \
				case ScriptFieldType::EnumType: *(Type*)valuePtr = fieldValue; break

			switch (scriptField.Type)
			{
				READ_FIELD_DATA(Float, float);
				READ_FIELD_DATA(Float2, glm::vec2);
				READ_FIELD_DATA(Float3, glm::vec3);
				READ_FIELD_DATA(Float4, glm::vec4);
				READ_FIELD_DATA(Int, int);
				READ_FIELD_DATA(Int2, glm::ivec2);
				READ_FIELD_DATA(Int3, glm::ivec3);
				READ_FIELD_DATA(Int4, glm::ivec4);
				READ_FIELD_DATA(Bool, bool);
				READ_FIELD_DATA(String, std::string);
			}
		}
	}
}
