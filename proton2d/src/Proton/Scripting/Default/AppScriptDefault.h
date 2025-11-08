#pragma once
#include "Proton/Scripting/AppScript.h"
#include "Proton/Scripting/ScriptFactory.h"

namespace proton {

	class AppScriptDefault : public AppScript
	{
	public:
		APP_SCRIPT_CLASS(AppScriptDefault);

		AppScriptDefault() = default;
		virtual ~AppScriptDefault() = default;
	};

}