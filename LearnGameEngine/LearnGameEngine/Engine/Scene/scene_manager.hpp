#pragma once

#include <memory>
#include "Scene/scene.hpp"

namespace Engine
{
	class SceneManager
	{
	public:
		SceneManager();
		~SceneManager();


		Scene& GetActiveScene();
		const Scene& GetActiveScene() const;

		void SetActiveScene(std::unique_ptr<Scene> scene);

		bool HasActiveScene() const { return static_cast<bool>(m_ActiveScene); }

		//load scene,reload,async load...
	private:
		std::unique_ptr<Scene> m_ActiveScene;
	};
}