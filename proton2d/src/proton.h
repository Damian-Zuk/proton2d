#pragma once

#include "Proton/Core/Base.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Core/AppLayer.h"
#include "Proton/Core/Input.h"
#include "Proton/Core/Timer.h"

#include "Proton/Debug/Assert.h"
#include "Proton/Debug/Instrumentor.h"

#include "Proton/Utils/Random.h"
#include "Proton/Utils/Utils.h"

#include "Proton/Assets/AssetManager.h"
#include "Proton/Assets/SceneSerializer.h"

#include "Proton/Events/KeyEvents.h"
#include "Proton/Events/MouseEvents.h"
#include "Proton/Events/WindowEvents.h"

#include "Proton/Scene/SceneManager.h"
#include "Proton/Scene/PrefabManager.h"

#include "Proton/Scripting/EntityScript.h"
#include "Proton/Scripting/GameModeBase.h"
#include "Proton/Scripting/GameModeFactory.h"

#include "Proton/Network/Common/NetworkManager.h"

#ifdef PT_EDITOR
#include <imgui/imgui.h>
#endif
