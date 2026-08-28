#include "asset_browser_panel.hpp"

#include <algorithm>
#include <cctype>
#include <system_error>

#include <imgui.h>

namespace Ludus::Editor
{
	namespace
	{
		std::string Lowercase(std::string value)
		{
			for (char& character : value)
				character = static_cast<char>(
					std::tolower(static_cast<unsigned char>(character)));
			return value;
		}

		std::vector<std::filesystem::path> ListDirectories(
			const std::filesystem::path& directory)
		{
			std::vector<std::filesystem::path> directories;
			std::error_code error;
			std::filesystem::directory_iterator iterator(
				directory,
				std::filesystem::directory_options::skip_permission_denied,
				error);
			if (error)
				return directories;

			for (; iterator != std::filesystem::directory_iterator();
				iterator.increment(error))
			{
				if (error)
					break;

				std::error_code typeError;
				if (iterator->is_directory(typeError) && !typeError)
					directories.push_back(iterator->path());
			}

			std::sort(
				directories.begin(),
				directories.end(),
				[](const std::filesystem::path& left,
					const std::filesystem::path& right)
				{
					return Lowercase(left.filename().string()) <
						Lowercase(right.filename().string());
				});
			return directories;
		}

		std::string RelativePath(
			const std::filesystem::path& root,
			const std::filesystem::path& path)
		{
			std::error_code error;
			const std::filesystem::path relative =
				std::filesystem::relative(path, root, error);
			if (error || relative.empty() || relative == ".")
				return root.filename().string();
			return root.filename().string() + "/" + relative.generic_string();
		}

		std::string Ellipsize(std::string value, float maxWidth)
		{
			if (ImGui::CalcTextSize(value.c_str()).x <= maxWidth)
				return value;

			while (value.size() > 1 &&
				ImGui::CalcTextSize((value + "...").c_str()).x > maxWidth)
				value.pop_back();
			return value + "...";
		}
	}

	void AssetBrowserPanel::Draw(const std::filesystem::path& assetsRoot)
	{
		const std::filesystem::path normalizedRoot = assetsRoot.lexically_normal();
		if (_assetsRoot != normalizedRoot)
		{
			_assetsRoot = normalizedRoot;
			_currentDirectory = _assetsRoot;
			_selectedPath.clear();
			_refreshRequested = true;
		}

		if (!ImGui::Begin("Asset Browser"))
		{
			ImGui::End();
			return;
		}

		std::error_code rootError;
		const bool validRoot = std::filesystem::exists(_assetsRoot, rootError) &&
			std::filesystem::is_directory(_assetsRoot, rootError);
		if (!validRoot)
		{
			ImGui::TextDisabled(
				"Assets directory not found: %s", _assetsRoot.string().c_str());
			ImGui::End();
			return;
		}

		if (ImGui::Button("Refresh"))
			_refreshRequested = true;
		ImGui::SameLine();

		const std::string breadcrumb = RelativePath(_assetsRoot, _currentDirectory);
		ImGui::TextDisabled("%s", breadcrumb.c_str());

		ImGui::Separator();
		ImGui::SetNextItemWidth(220.0f);
		ImGui::InputTextWithHint(
			"##AssetSearch",
			"Search assets...",
			_search,
			IM_ARRAYSIZE(_search));
		ImGui::SameLine();

		ImGui::SetNextItemWidth(135.0f);
		if (ImGui::BeginCombo("##AssetFilter", FilterName(_filter)))
		{
			for (int index = 0; index <= static_cast<int>(Filter::Scenes); ++index)
			{
				const Filter candidate = static_cast<Filter>(index);
				const bool selected = candidate == _filter;
				if (ImGui::Selectable(FilterName(candidate), selected))
					_filter = candidate;
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if (_refreshRequested)
		{
			Refresh();
			_refreshRequested = false;
		}

		const float availableHeight = std::max(
			120.0f, ImGui::GetContentRegionAvail().y - 68.0f);
		const float availableWidth = ImGui::GetContentRegionAvail().x;
		const float folderWidth = std::min(
			190.0f, std::max(145.0f, availableWidth * 0.26f));

		const bool foldersVisible = ImGui::BeginChild(
			"AssetBrowserFolders",
			ImVec2(folderWidth, availableHeight),
			true);
		if (foldersVisible)
		{
			ImGui::TextDisabled("Folders");
			ImGui::Separator();
			DrawFolderTree(_assetsRoot, true);
		}
		ImGui::EndChild();
		ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x);

		const bool gridVisible = ImGui::BeginChild(
			"AssetBrowserGrid",
			ImVec2(0.0f, availableHeight),
			true);
		if (gridVisible)
			DrawAssetGrid();
		ImGui::EndChild();

		DrawSelectedDetails();
		ImGui::End();
	}

	void AssetBrowserPanel::DrawFolderTree(
		const std::filesystem::path& directory,
		bool root)
	{
		const std::string path = directory.string();
		const std::string name = directory.filename().string();
		const ImGuiTreeNodeFlags flags =
			ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			(directory == _currentDirectory ? ImGuiTreeNodeFlags_Selected : 0) |
			(root ? ImGuiTreeNodeFlags_DefaultOpen : 0);

		ImGui::PushID(path.c_str());
		const bool open = ImGui::TreeNodeEx(
			"##Folder",
			flags,
			"%s",
			name.empty() ? "assets" : name.c_str());
		if (ImGui::IsItemClicked())
			NavigateTo(directory);

		if (open)
		{
			for (const std::filesystem::path& child : ListDirectories(directory))
				DrawFolderTree(child, false);
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	void AssetBrowserPanel::DrawAssetGrid()
	{
		constexpr float tileWidth = 96.0f;
		constexpr float tileHeight = 86.0f;
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float availableWidth = ImGui::GetContentRegionAvail().x;
		const int columns = std::max(
			1,
			static_cast<int>((availableWidth + spacing) / (tileWidth + spacing)));

		int column = 0;
		bool displayedEntry = false;
		if (_currentDirectory != _assetsRoot)
		{
			const Entry parent{ _currentDirectory.parent_path(), true };
			DrawAssetTile(parent, tileWidth, tileHeight, "..");
			displayedEntry = true;
			++column;
			if (column < columns)
				ImGui::SameLine();
			else
				column = 0;
		}

		for (const Entry& entry : _entries)
		{
			if (!entry.directory &&
				(!MatchesFilter(entry.path) || !MatchesSearch(entry.path)))
				continue;

			displayedEntry = true;
			DrawAssetTile(entry, tileWidth, tileHeight);
			++column;
			if (column < columns)
				ImGui::SameLine();
			else
				column = 0;
		}

		if (!displayedEntry)
			ImGui::TextDisabled("No matching assets");
	}

	void AssetBrowserPanel::DrawAssetTile(
		const Entry& entry,
		float width,
		float height,
		const char* displayName)
	{
		const std::string path = entry.path.string();
		const std::string name = displayName
			? displayName
			: entry.path.filename().string();
		const AssetType type = GetAssetType(entry.path, entry.directory);
		ImGui::PushID(path.c_str());

		const ImVec2 tileMinimum = ImGui::GetCursorScreenPos();
		const bool pressed = ImGui::InvisibleButton("##AssetTile", { width, height });
		const bool hovered = ImGui::IsItemHovered();
		if (pressed)
			_selectedPath = path;
		if (entry.directory &&
			ImGui::IsItemHovered() &&
			ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			NavigateTo(entry.path);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			_selectedPath = path;

		const bool selected = _selectedPath == path;
		const ImVec2 tileMaximum{
			tileMinimum.x + width,
			tileMinimum.y + height };
		const ImVec4 background = selected
			? ImVec4(0.18f, 0.34f, 0.55f, 1.0f)
			: hovered
			? ImVec4(0.16f, 0.19f, 0.24f, 1.0f)
			: ImVec4(0.11f, 0.13f, 0.16f, 1.0f);
		const ImU32 backgroundColor =
			ImGui::ColorConvertFloat4ToU32(background);
		const ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(
			selected
				? ImVec4(0.35f, 0.65f, 0.95f, 1.0f)
				: ImVec4(0.20f, 0.23f, 0.28f, 1.0f));
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(tileMinimum, tileMaximum, backgroundColor, 5.0f);
		drawList->AddRect(tileMinimum, tileMaximum, borderColor, 5.0f);

		const ImVec2 center{
			tileMinimum.x + width * 0.5f,
			tileMinimum.y + 29.0f };
		const ImU32 iconColor = ImGui::ColorConvertFloat4ToU32(
			type == AssetType::Folder ? ImVec4(0.96f, 0.72f, 0.27f, 1.0f) :
			type == AssetType::Texture ? ImVec4(0.35f, 0.78f, 0.55f, 1.0f) :
			type == AssetType::Model ? ImVec4(0.47f, 0.66f, 0.95f, 1.0f) :
			type == AssetType::Shader ? ImVec4(0.83f, 0.48f, 0.91f, 1.0f) :
			type == AssetType::Scene ? ImVec4(0.95f, 0.53f, 0.36f, 1.0f) :
			ImVec4(0.62f, 0.67f, 0.73f, 1.0f));

		switch (type)
		{
		case AssetType::Folder:
			drawList->AddRectFilled(
				{ center.x - 19.0f, center.y - 10.0f },
				{ center.x + 19.0f, center.y + 13.0f },
				iconColor,
				4.0f);
			drawList->AddRectFilled(
				{ center.x - 15.0f, center.y - 15.0f },
				{ center.x - 1.0f, center.y - 9.0f },
				iconColor,
				3.0f);
			break;
		case AssetType::Texture:
			drawList->AddRectFilled(
				{ center.x - 18.0f, center.y - 15.0f },
				{ center.x + 18.0f, center.y + 15.0f },
				ImGui::ColorConvertFloat4ToU32(ImVec4(0.08f, 0.10f, 0.12f, 1.0f)),
				3.0f);
			drawList->AddRect(
				{ center.x - 18.0f, center.y - 15.0f },
				{ center.x + 18.0f, center.y + 15.0f },
				iconColor,
				3.0f);
			drawList->AddTriangleFilled(
				{ center.x - 14.0f, center.y + 11.0f },
				{ center.x - 2.0f, center.y - 3.0f },
				{ center.x + 5.0f, center.y + 11.0f },
				iconColor);
			drawList->AddCircleFilled(
				{ center.x + 9.0f, center.y - 7.0f }, 3.0f, iconColor);
			break;
		case AssetType::Model:
			drawList->AddLine(
				{ center.x, center.y - 17.0f },
				{ center.x + 17.0f, center.y - 7.0f },
				iconColor,
				2.0f);
			drawList->AddLine(
				{ center.x + 17.0f, center.y - 7.0f },
				{ center.x + 17.0f, center.y + 12.0f },
				iconColor,
				2.0f);
			drawList->AddLine(
				{ center.x + 17.0f, center.y + 12.0f },
				{ center.x, center.y + 21.0f },
				iconColor,
				2.0f);
			drawList->AddLine(
				{ center.x, center.y + 21.0f },
				{ center.x - 17.0f, center.y + 12.0f },
				iconColor,
				2.0f);
			drawList->AddLine(
				{ center.x - 17.0f, center.y + 12.0f },
				{ center.x - 17.0f, center.y - 7.0f },
				iconColor,
				2.0f);
			drawList->AddLine(
				{ center.x - 17.0f, center.y - 7.0f },
				{ center.x, center.y - 17.0f },
				iconColor,
				2.0f);
			drawList->AddLine(
				{ center.x, center.y - 17.0f },
				{ center.x, center.y + 2.0f },
				iconColor,
				2.0f);
			drawList->AddLine(
				{ center.x - 17.0f, center.y - 7.0f },
				{ center.x, center.y + 2.0f },
				iconColor,
				2.0f);
			drawList->AddLine(
				{ center.x + 17.0f, center.y - 7.0f },
				{ center.x, center.y + 2.0f },
				iconColor,
				2.0f);
			break;
		case AssetType::Shader:
		case AssetType::Scene:
		case AssetType::Other:
			drawList->AddRectFilled(
				{ center.x - 16.0f, center.y - 18.0f },
				{ center.x + 16.0f, center.y + 18.0f },
				iconColor,
				3.0f);
			drawList->AddTriangleFilled(
				{ center.x + 5.0f, center.y - 18.0f },
				{ center.x + 16.0f, center.y - 7.0f },
				{ center.x + 5.0f, center.y - 7.0f },
				ImGui::ColorConvertFloat4ToU32(background));
			if (type == AssetType::Shader)
			{
				drawList->AddLine(
					{ center.x - 9.0f, center.y - 3.0f },
					{ center.x + 9.0f, center.y - 3.0f },
					ImGui::ColorConvertFloat4ToU32(background),
					2.0f);
				drawList->AddLine(
					{ center.x - 9.0f, center.y + 5.0f },
					{ center.x + 9.0f, center.y + 5.0f },
					ImGui::ColorConvertFloat4ToU32(background),
					2.0f);
			}
			break;
		}

		const std::string label = Ellipsize(name, width - 10.0f);
		const ImVec2 labelSize = ImGui::CalcTextSize(label.c_str());
		drawList->AddText(
			{ tileMinimum.x + (width - labelSize.x) * 0.5f,
				tileMaximum.y - 19.0f },
			ImGui::ColorConvertFloat4ToU32(ImVec4(0.88f, 0.90f, 0.93f, 1.0f)),
			label.c_str());

		if (ImGui::BeginPopupContextItem("AssetContext"))
		{
			_selectedPath = path;
			ImGui::TextDisabled("%s", name.c_str());
			ImGui::Separator();
			if (entry.directory && ImGui::MenuItem("Open Folder"))
				NavigateTo(entry.path);
			if (ImGui::MenuItem("Copy Path"))
				ImGui::SetClipboardText(path.c_str());
			ImGui::EndPopup();
		}

		if (hovered)
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted(name.c_str());
			ImGui::TextDisabled("%s", path.c_str());
			ImGui::EndTooltip();
		}
		ImGui::PopID();
	}

	void AssetBrowserPanel::DrawSelectedDetails()
	{
		ImGui::Separator();
		if (_selectedPath.empty())
		{
			ImGui::TextDisabled("No asset selected");
			return;
		}

		const std::filesystem::path selectedPath(_selectedPath);
		const AssetType type = GetAssetType(
			selectedPath,
			std::filesystem::is_directory(selectedPath));
		const std::string name = selectedPath.filename().string();
		const std::string relative = RelativePath(_assetsRoot, selectedPath);
		ImGui::Text("%s", name.c_str());
		ImGui::SameLine();
		ImGui::TextDisabled("(%s)", TypeName(type));
		ImGui::TextDisabled("Path: %s", relative.c_str());

		std::error_code error;
		if (std::filesystem::is_regular_file(selectedPath, error))
		{
			std::error_code sizeError;
			const uintmax_t size = std::filesystem::file_size(selectedPath, sizeError);
			if (!sizeError)
				ImGui::TextDisabled(
					"Size: %llu bytes",
					static_cast<unsigned long long>(size));
		}
	}

	void AssetBrowserPanel::NavigateTo(
		const std::filesystem::path& directory)
	{
		std::error_code error;
		if (!std::filesystem::is_directory(directory, error) || error)
			return;

		_currentDirectory = directory.lexically_normal();
		_selectedPath.clear();
		_refreshRequested = true;

	}

	void AssetBrowserPanel::Refresh()
	{
		_entries.clear();
		if (_currentDirectory.empty())
			return;

		std::error_code error;
		std::filesystem::directory_iterator iterator(
			_currentDirectory,
			std::filesystem::directory_options::skip_permission_denied,
			error);
		if (error)
			return;

		for (; iterator != std::filesystem::directory_iterator();
			iterator.increment(error))
		{
			if (error)
				break;

			const std::filesystem::directory_entry& candidate = *iterator;
			std::error_code typeError;
			const bool directory = candidate.is_directory(typeError);
			if (typeError)
				continue;
			if (!directory)
			{
				typeError.clear();
				if (!candidate.is_regular_file(typeError) || typeError)
					continue;
			}
			_entries.push_back({ candidate.path(), directory });
		}

		std::sort(
			_entries.begin(),
			_entries.end(),
			[](const Entry& left, const Entry& right)
			{
				if (left.directory != right.directory)
					return left.directory;
				return Lowercase(left.path.filename().string()) <
					Lowercase(right.path.filename().string());
			});

		if (!_selectedPath.empty())
		{
			std::error_code selectedError;
			if (!std::filesystem::exists(_selectedPath, selectedError) || selectedError)
				_selectedPath.clear();
		}
	}

	bool AssetBrowserPanel::MatchesSearch(const std::filesystem::path& path) const
	{
		const std::string query = Lowercase(_search);
		return query.empty() ||
			Lowercase(path.filename().string()).find(query) != std::string::npos;
	}

	bool AssetBrowserPanel::MatchesFilter(const std::filesystem::path& path) const
	{
		const AssetType type = GetAssetType(path, false);
		switch (_filter)
		{
		case Filter::All:
			return true;
		case Filter::Textures:
			return type == AssetType::Texture;
		case Filter::Models:
			return type == AssetType::Model;
		case Filter::Shaders:
			return type == AssetType::Shader;
		case Filter::Scenes:
			return type == AssetType::Scene;
		}
		return true;
	}

	AssetBrowserPanel::AssetType AssetBrowserPanel::GetAssetType(
		const std::filesystem::path& path,
		bool directory) noexcept
	{
		if (directory)
			return AssetType::Folder;

		const std::string extension = Lowercase(path.extension().string());
		if (extension == ".png" || extension == ".jpg" ||
			extension == ".jpeg" || extension == ".bmp" ||
			extension == ".tga" || extension == ".hdr" ||
			extension == ".dds")
			return AssetType::Texture;
		if (extension == ".obj" || extension == ".fbx" ||
			extension == ".gltf" || extension == ".glb" ||
			extension == ".mtl")
			return AssetType::Model;
		if (extension == ".vert" || extension == ".frag" ||
			extension == ".spv" || extension == ".comp" ||
			extension == ".glsl" || extension == ".geom")
			return AssetType::Shader;
		if (extension == ".lscene")
			return AssetType::Scene;
		return AssetType::Other;
	}

	const char* AssetBrowserPanel::FilterName(Filter filter) noexcept
	{
		switch (filter)
		{
		case Filter::All: return "All";
		case Filter::Textures: return "Textures";
		case Filter::Models: return "Models";
		case Filter::Shaders: return "Shaders";
		case Filter::Scenes: return "Scenes";
		}
		return "All";
	}

	const char* AssetBrowserPanel::TypeName(AssetType type) noexcept
	{
		switch (type)
		{
		case AssetType::Folder: return "Folder";
		case AssetType::Texture: return "Texture";
		case AssetType::Model: return "Model";
		case AssetType::Shader: return "Shader";
		case AssetType::Scene: return "Scene";
		case AssetType::Other: return "File";
		}
		return "File";
	}
}
