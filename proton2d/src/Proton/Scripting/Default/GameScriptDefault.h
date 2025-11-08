#pragma once
#include "Proton/Scripting/GameScript.h"
#include "Proton/Scripting/ScriptFactory.h"

namespace proton {

	class GameScriptDefault : public GameScript
	{
	public:
		GAME_SCRIPT_CLASS(GameScriptDefault);

		GameScriptDefault() = default;
		virtual ~GameScriptDefault() = default;
	};

}