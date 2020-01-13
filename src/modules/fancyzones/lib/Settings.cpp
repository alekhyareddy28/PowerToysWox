#include "pch.h"
#include <common/common.h>
#include <common/settings_objects.h>
#include "lib/Settings.h"
#include "lib/FancyZones.h"
#include "trace.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

struct FancyZonesSettings : winrt::implements<FancyZonesSettings, IFancyZonesSettings>
{
public:
    FancyZonesSettings(HINSTANCE hinstance, PCWSTR name)
        : m_hinstance(hinstance)
        , m_name(name)
    {
        LoadSettings(name, true /*fromFile*/);
    }

    IFACEMETHODIMP_(void) SetCallback(IFancyZonesCallback* callback) { m_callback = callback; }
    IFACEMETHODIMP_(bool) GetConfig(_Out_ PWSTR buffer, _Out_ int *buffer_sizeg) noexcept;
    IFACEMETHODIMP_(void) SetConfig(PCWSTR config) noexcept;
    IFACEMETHODIMP_(void) CallCustomAction(PCWSTR action) noexcept;
    IFACEMETHODIMP_(Settings) GetSettings() noexcept { return m_settings; }

private:
    void LoadSettings(PCWSTR config, bool fromFile) noexcept;
    void SaveSettings() noexcept;

    IFancyZonesCallback* m_callback{};
    const HINSTANCE m_hinstance;
    PCWSTR m_name{};

    Settings m_settings;

    struct
    {
        PCWSTR name;
        bool* value;
        int resourceId;
    } m_configBools[8] = {
        { GET_RESOURCE_STRING(IDS_SHIFTDRAG).c_str(), &m_settings.shiftDrag, IDS_SETTING_DESCRIPTION_SHIFTDRAG },
        { GET_RESOURCE_STRING(IDS_OVERRIDE_HOTKEYS).c_str(), &m_settings.overrideSnapHotkeys, IDS_SETTING_DESCRIPTION_OVERRIDE_SNAP_HOTKEYS },
        { GET_RESOURCE_STRING(IDS_ZONESET_CHANGE_FLASHZONES).c_str(), &m_settings.zoneSetChange_flashZones, IDS_SETTING_DESCRIPTION_ZONESETCHANGE_FLASHZONES },
        { GET_RESOURCE_STRING(IDS_DISPLAY_CHANGE_MOVEWINDOWS).c_str(), &m_settings.displayChange_moveWindows, IDS_SETTING_DESCRIPTION_DISPLAYCHANGE_MOVEWINDOWS },
        { GET_RESOURCE_STRING(IDS_ZONESET_CHANGE_MOVEWINDOWS).c_str(), &m_settings.zoneSetChange_moveWindows, IDS_SETTING_DESCRIPTION_ZONESETCHANGE_MOVEWINDOWS },
        { GET_RESOURCE_STRING(IDS_VIRTUALDESKTOP_MOVEWINDOWS).c_str(), &m_settings.virtualDesktopChange_moveWindows, IDS_SETTING_DESCRIPTION_VIRTUALDESKTOPCHANGE_MOVEWINDOWS },
        { GET_RESOURCE_STRING(IDS_APP_LASTZONE_MOVEWINDOWS).c_str(), &m_settings.appLastZone_moveWindows, IDS_SETTING_DESCRIPTION_APPLASTZONE_MOVEWINDOWS },
        { GET_RESOURCE_STRING(IDS_EDITOR_STARTUP_SCREEN).c_str(), &m_settings.use_cursorpos_editor_startupscreen, IDS_SETTING_DESCRIPTION_USE_CURSORPOS_EDITOR_STARTUPSCREEN },
    };

    const std::wstring m_zoneHiglightName = GET_RESOURCE_STRING(IDS_ZONE_HIGHLIGHT_COLOR).c_str();
    const std::wstring m_editorHotkeyName = GET_RESOURCE_STRING(IDS_EDITOR_HOTKEY).c_str();
    const std::wstring m_excludedAppsName = GET_RESOURCE_STRING(IDS_EXCLUDED_APPS).c_str();
    const std::wstring m_zoneHighlightOpacity = GET_RESOURCE_STRING(IDS_HIGHLIGHT_OPACITY).c_str();
};

IFACEMETHODIMP_(bool) FancyZonesSettings::GetConfig(_Out_ PWSTR buffer, _Out_ int *buffer_size) noexcept
{
    PowerToysSettings::Settings settings(m_hinstance, m_name);

    // Pass a string literal or a resource id to Settings::set_description().
    settings.set_description(IDS_SETTING_DESCRIPTION);
    settings.set_icon_key(GET_RESOURCE_STRING(IDS_SET_ICON_KEY).c_str());
    settings.set_overview_link(GET_RESOURCE_STRING(IDS_OVERVIEW_LINK).c_str());
    settings.set_video_link(GET_RESOURCE_STRING(IDS_VIDEO_LINK).c_str());

    // Add a custom action property. When using this settings type, the "PowertoyModuleIface::call_custom_action()"
    // method should be overriden as well.
    settings.add_custom_action(
        GET_RESOURCE_STRING(IDS_TOGGLE_EDITOR).c_str(), // action name.
        IDS_SETTING_LAUNCH_EDITOR_LABEL,
        IDS_SETTING_LAUNCH_EDITOR_BUTTON,
        IDS_SETTING_LAUNCH_EDITOR_DESCRIPTION
    );
    settings.add_hotkey(m_editorHotkeyName, IDS_SETTING_LAUNCH_EDITOR_HOTKEY_LABEL, m_settings.editorHotkey);

    for (auto const& setting : m_configBools)
    {
        settings.add_bool_toogle(setting.name, setting.resourceId, *setting.value);
    }

    settings.add_int_spinner(m_zoneHighlightOpacity, IDS_SETTINGS_HIGHLIGHT_OPACITY, m_settings.zoneHighlightOpacity, 0, 100, 1);
    settings.add_color_picker(m_zoneHiglightName, IDS_SETTING_DESCRIPTION_ZONEHIGHLIGHTCOLOR, m_settings.zoneHightlightColor);
    settings.add_multiline_string(m_excludedAppsName, IDS_SETTING_EXCLCUDED_APPS_DESCRIPTION, m_settings.excludedApps);

    return settings.serialize_to_buffer(buffer, buffer_size);
}

IFACEMETHODIMP_(void) FancyZonesSettings::SetConfig(PCWSTR config) noexcept try
{
    LoadSettings(config, false /*fromFile*/);
    SaveSettings();
    if (m_callback)
    {
        m_callback->SettingsChanged();
    }
    Trace::SettingsChanged(m_settings);
}
CATCH_LOG();

IFACEMETHODIMP_(void) FancyZonesSettings::CallCustomAction(PCWSTR action) noexcept try
{
    // Parse the action values, including name.
    PowerToysSettings::CustomActionObject action_object =
        PowerToysSettings::CustomActionObject::from_json_string(action);

    if (m_callback && action_object.get_name() == GET_RESOURCE_STRING(IDS_TOGGLE_EDITOR).c_str())
    {
        m_callback->ToggleEditor();
    }
}
CATCH_LOG();

void FancyZonesSettings::LoadSettings(PCWSTR config, bool fromFile) noexcept try
{
    PowerToysSettings::PowerToyValues values = fromFile ?
        PowerToysSettings::PowerToyValues::load_from_settings_file(m_name) :
        PowerToysSettings::PowerToyValues::from_json_string(config);

    for (auto const& setting : m_configBools)
    {
        if (const auto val = values.get_bool_value(setting.name))
        {
            *setting.value = *val;
        }
    }

    if (auto val = values.get_string_value(m_zoneHiglightName))
    {
        m_settings.zoneHightlightColor = std::move(*val);
    }

    if (const auto val = values.get_json(m_editorHotkeyName))
    {
        m_settings.editorHotkey = PowerToysSettings::HotkeyObject::from_json(*val);
    }

    if (auto val = values.get_string_value(m_excludedAppsName))
    {
        m_settings.excludedApps = std::move(*val);
        m_settings.excludedAppsArray.clear();
        auto excludedUppercase = m_settings.excludedApps;
        CharUpperBuffW(excludedUppercase.data(), (DWORD)excludedUppercase.length());
        std::wstring_view view(excludedUppercase);
        while (view.starts_with('\n') || view.starts_with('\r'))
        {
            view.remove_prefix(1);
        }
        while (!view.empty())
        {
            auto pos = (std::min)(view.find_first_of(L"\r\n"), view.length());
            m_settings.excludedAppsArray.emplace_back(view.substr(0, pos));
            view.remove_prefix(pos);
            while (view.starts_with('\n') || view.starts_with('\r'))
            {
                view.remove_prefix(1);
            }
        }
    }

    if (auto val = values.get_int_value(m_zoneHighlightOpacity))
    {
        m_settings.zoneHighlightOpacity = *val;
    }
}
CATCH_LOG();

void FancyZonesSettings::SaveSettings() noexcept try
{
    PowerToysSettings::PowerToyValues values(m_name);

    for (auto const& setting : m_configBools)
    {
        values.add_property(setting.name, *setting.value);
    }

    values.add_property(m_zoneHiglightName, m_settings.zoneHightlightColor);
    values.add_property(m_zoneHighlightOpacity, m_settings.zoneHighlightOpacity);
    values.add_property(m_editorHotkeyName, m_settings.editorHotkey.get_json());
    values.add_property(m_excludedAppsName, m_settings.excludedApps);

    values.save_to_settings_file();
}
CATCH_LOG();

winrt::com_ptr<IFancyZonesSettings> MakeFancyZonesSettings(HINSTANCE hinstance, PCWSTR name) noexcept
{
    return winrt::make_self<FancyZonesSettings>(hinstance, name);
}