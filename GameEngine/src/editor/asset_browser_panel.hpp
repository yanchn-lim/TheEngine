#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Ludus::Editor
{
	class AssetBrowserPanel
	{
	public:
		void Draw(const std::filesystem::path& assetsRoot);

	private:
		enum class Filter
		{
			All,
			Textures,
			Models,
			Shaders,
			Scenes
		};
		enum class AssetType
		{
			Folder,
			Texture,
			Model,
			Shader,
			Scene,
			Other
		};

		struct Entry
		{
			std::filesystem::path path;
			bool directory = false;
		};

		void Refresh();
		void DrawFolderTree(const std::filesystem::path& directory, bool root);
		void DrawAssetGrid();
		void DrawAssetTile(
			const Entry& entry,
			float width,
			float height,
			const char* displayName = nullptr);
		void DrawSelectedDetails();
		void NavigateTo(const std::filesystem::path& directory);
		bool MatchesSearch(const std::filesystem::path& path) const;
		bool MatchesFilter(const std::filesystem::path& path) const;
		static AssetType GetAssetType(
			const std::filesystem::path& path,
			bool directory) noexcept;
		static const char* FilterName(Filter filter) noexcept;
		static const char* TypeName(AssetType type) noexcept;

		std::filesystem::path _assetsRoot;
		std::filesystem::path _currentDirectory;
		std::vector<Entry> _entries;
		std::string _selectedPath;
		char _search[128]{};
		Filter _filter = Filter::All;
		bool _refreshRequested = true;
	};
}
