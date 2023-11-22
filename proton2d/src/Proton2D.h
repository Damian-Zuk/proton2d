#pragma once

#include "proton/Core/Application.h"
#include "proton/Core/Input.h"

#include "proton/Debug/Log.h"
#include "proton/Debug/Assert.h"
#include "proton/Debug/Instrumentor.h"

#include "proton/Utils/Random.h"
#include "proton/Utils/Utils.h"

#include "proton/Assets/AssetManager.h"
#include "proton/Assets/SceneSerializer.h"

#include "proton/Events/KeyEvents.h"
#include "proton/Events/MouseEvents.h"
#include "proton/Events/WindowEvents.h"

#include "proton/Scene/Entity.h"
#include "proton/Scene/Components.h"
#include "proton/Scene/EntityScript.h"
#include "proton/Scene/SceneManager.h"
#include "proton/Scene/PrefabManager.h"

#include "proton/Graphics/SpriteAnimation.h"

#ifdef PT_EDITOR
#include <imgui/imgui.h>
#endif
