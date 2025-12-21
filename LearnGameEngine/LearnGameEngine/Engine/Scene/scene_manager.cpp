#pragma once

#include "Scene/scene_manager.hpp"
#include <cassert>

namespace Engine
{
	SceneManager::SceneManager() = default;
	SceneManager::~SceneManager() = default;

	Scene& SceneManager::GetActiveScene()
	{
		assert(m_ActiveScene && "No active scene set in SceneManager");
		return *m_ActiveScene;
	}

	const Scene& SceneManager::GetActiveScene() const
	{
		assert(m_ActiveScene && "No active scene set in SceneManager");
		return *m_ActiveScene;
	}

	void SceneManager::SetActiveScene(std::unique_ptr<Scene> scene)
	{
		m_ActiveScene = std::move(scene);
	}
}