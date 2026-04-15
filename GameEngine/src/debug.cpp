#include "debug.hpp"
#include "imgui.h"

void DebugConsole::PushLog(LogLevel level, std::string message)
{
    // Also mirror to stdout with a prefix
    const char* prefix = "";
    switch (level)
    {
    case LogLevel::VERBOSE: prefix = "[VERBOSE] "; break;
    case LogLevel::INFO:    prefix = "[INFO]    "; break;
    case LogLevel::WARNING: prefix = "[WARNING] "; break;
    case LogLevel::ERROR:   prefix = "[ERROR]   "; break;
    }
    std::cout << prefix << message << "\n";

    _entries.Push({ level,std::move(message) });
    _scrollToBottom = true;
}

void DebugConsole::Clear()
{
    _entries = {};
}

void DebugConsole::Draw()
{
    ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Console"))
    {
        ImGui::End();
        return;
    }

    // -- Toolbar --
    if (ImGui::Button("Clear"))
        Clear();

    // Future: add filter/search widgets here without restructuring anything

    ImGui::Separator();

    // -- Log body --
    // Reserve space so the scrolling region doesn't overlap a future toolbar at bottom
    const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollRegion", ImVec2(0, -footerHeight), false, ImGuiWindowFlags_HorizontalScrollbar);

    const size_t count = _entries.count;
    const size_t cap = MAX_LOG_ENTRIES;

    for (size_t i = 0; i < count; ++i)
    {
        // Walk chronologically: oldest first
        const LogEntry& e = _entries[((_entries.head + cap - count + i) % cap)];

        ImVec4 color;
        switch (e.level)
        {
        case LogLevel::VERBOSE: color = ImVec4(0.6f, 0.6f, 0.6f, 1.f); break;
        case LogLevel::INFO:    color = ImVec4(1.f, 1.f, 1.f, 1.f); break;
        case LogLevel::WARNING: color = ImVec4(1.f, 0.8f, 0.f, 1.f); break;
        case LogLevel::ERROR:   color = ImVec4(1.f, 0.3f, 0.3f, 1.f); break;
        default:                color = ImVec4(1.f, 1.f, 1.f, 1.f); break;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(e.message.c_str());
        ImGui::PopStyleColor();
    }

    // Auto-scroll only if already at the bottom
    if (_scrollToBottom && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    _scrollToBottom = false;

    ImGui::EndChild();
    ImGui::End();
}