#include "ptpch.h"
#include "Proton/Utils/Utils.h"
#include "Proton/Utils/Random.h"
#include "NickGenerator.h"

namespace proton {

	static const std::array<std::string, 50> s_ExamplePlayerNicknames = {
		"AppleCrunch","BerryBlitz","CarrotCraze","MangoMania","PeachPunch","RadishRider","KiwiKicker","TomatoTwist","LemonLancer",
		"PotatoPaladin","GrapeGunner","OliveOverlord","OnionOracle","CherryChase","MelonMarauder","CucumberCrush","PineappleProwl",
		"StrawberryStorm","AvocadoAssault","BroccoliBrigade","PepperPioneer","ZucchiniZealot","LettuceLancer","BananaBrawler",
		"CoconutCommander","OrangeOutlaw","SpinachSentry","PumpkinPatrol","TurnipTitan","BeetleBuster","FigFury","NectarineNinja",
		"PlumPirate","ApricotAce","PapayaPaladin","SquashSquire","RadicchioRider","PearProwler","PersimmonPioneer","QuinceQuester",
		"RutabagaRogue","DateDestroyer","LycheeLancer","MulberryMystic","ElderberryEcho","PomegranatePunch","StarfruitSlicer",
		"DurianDominator","GuavaGuardian","GooseberryGlider"
	};

	std::string proton::GenerateRandomNickname()
	{
		uint32_t rng = Random::Int(1, 99);
		std::string rngStr = rng < 10 ? "0" + std::to_string(rng) : std::to_string(rng);
		return s_ExamplePlayerNicknames[Random::Int(0, 49)] + rngStr;
	}

}
