#pragma once

namespace proton {

	class Script;

	class ScriptEditWidget
	{
	public:
		ScriptEditWidget() = default;
		virtual ~ScriptEditWidget() = default;

		static void DrawFieldEdit(Script* script);

	};

}

