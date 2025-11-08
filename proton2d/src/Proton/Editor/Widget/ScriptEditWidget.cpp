#include "ptpch.h"
#include "Proton/Editor/Widget/ScriptEditWidget.h"
#include "Proton/Scripting/Script.h"

#include <imgui.h>

namespace proton {

	void ScriptEditWidget::DrawFieldEdit(Script* script)
	{
		for (auto& [fieldName, fieldData] : script->m_ScriptFields)
		{
			if (!script->m_ScriptFields.at(fieldName).ShowInEditor)
				continue;

			switch (fieldData.Type)
			{
			case ScriptFieldType::Float:
				ImGui::DragFloat(fieldName.c_str(), (float*)fieldData.ValuePtr, 0.01f);
				break;
			case ScriptFieldType::Float2:
				ImGui::DragFloat2(fieldName.c_str(), (float*)fieldData.ValuePtr, 0.01f);
				break;
			case ScriptFieldType::Float3:
				ImGui::DragFloat3(fieldName.c_str(), (float*)fieldData.ValuePtr, 0.01f);
				break;
			case ScriptFieldType::Float4:
				if (fieldName.find("Color") != fieldName.npos || fieldName.find("color") != fieldName.npos)
					ImGui::ColorEdit4(fieldName.c_str(), (float*)fieldData.ValuePtr);
				else
					ImGui::DragFloat4(fieldName.c_str(), (float*)fieldData.ValuePtr, 0.01f);
				break;

			case ScriptFieldType::Int:
				ImGui::DragInt(fieldName.c_str(), (int*)fieldData.ValuePtr);
				break;
			case ScriptFieldType::Int2:
				ImGui::DragInt2(fieldName.c_str(), (int*)fieldData.ValuePtr);
				break;
			case ScriptFieldType::Int3:
				ImGui::DragInt3(fieldName.c_str(), (int*)fieldData.ValuePtr);
				break;
			case ScriptFieldType::Int4:
				ImGui::DragInt4(fieldName.c_str(), (int*)fieldData.ValuePtr);
				break;

			case ScriptFieldType::Bool:
				ImGui::Checkbox(fieldName.c_str(), (bool*)fieldData.ValuePtr);
				break;

			case ScriptFieldType::String:
			{
				static char buffer[1024];
				strcpy_s(buffer, ((std::string*)fieldData.ValuePtr)->c_str());
				if (ImGui::InputText(fieldName.c_str(), buffer, 1024))
				{
					std::string& strRef = *(std::string*)fieldData.ValuePtr;
					strRef = buffer;
				}
				break;
			}
			}
		}
		script->OnImGuiRender();
	}

}

