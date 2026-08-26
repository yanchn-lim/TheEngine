#include "test_runner.hpp"

#include <cstdlib>

#include "debug/debug.hpp"
#include "ecs_tests.hpp"
#include "graphics_api_tests.hpp"
#include "lscene_parser_tests.hpp"
#include "scene_asset_loader_tests.hpp"
#include "scene_loader_tests.hpp"

namespace Tests
{
	namespace
	{
		bool Run(const char* name, bool (*test)())
		{
			const bool passed = test();
			if (passed)
				Debug::Log("[PASS] ", name);
			else
				Debug::LogError("[FAIL] ", name);
			return passed;
		}
	}

	int RunAllTests()
	{
		bool passed = true;
		passed = Run("ECS", RunEcsTests) && passed;
		passed = Run("LScene parser", RunLSceneParserTests) && passed;
		passed = Run("Scene assets", RunSceneAssetLoaderTests) && passed;
		passed = Run("Scene loading", RunSceneLoaderTests) && passed;
		passed = Run("Graphics API", RunGraphicsApiTests) && passed;
		return passed ? EXIT_SUCCESS : EXIT_FAILURE;
	}
}
