#include "pch.h"
#include "NewToy.h"

IFACEMETHODIMP_(void)
NewToyCOM::Run() noexcept
{
    std::unique_lock writeLock(m_lock);

    // Registers the window class
    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = s_WndProc;
    wcex.hInstance = m_hinstance;
    wcex.lpszClassName = L"SuperNewToy";
    RegisterClassExW(&wcex);

    // First SendInput is slower. Send 0x0(unassigned) to complete a dummy SendInput so that remaining SendInput is faster
    INPUT tempEv;
    tempEv.type = INPUT_KEYBOARD;
    tempEv.ki.wVk = 0x0;
    tempEv.ki.dwFlags = 0;
    tempEv.ki.time = 0;
    tempEv.ki.dwExtraInfo = 0;
    SendInput(1, &tempEv, sizeof(INPUT));
    // Creates the window
    m_window = CreateWindowExW(0, L"SuperNewToy", titleText, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, m_hinstance, this);
    // If window creation fails, return
    if (!m_window)
        return;

    // Win + /
    // Note: Cannot overwrite existing Windows shortcuts
    RegisterHotKey(m_window, 1, m_settings->newToyShowHotkey.get_modifiers(), m_settings->newToyShowHotkey.get_code());
    RegisterHotKey(m_window, 2, m_settings->newToyEditHotkey.get_modifiers(), m_settings->newToyEditHotkey.get_code());
}

IFACEMETHODIMP_(void)
NewToyCOM::Destroy() noexcept
{
    // Locks the window
    std::unique_lock writeLock(m_lock);
    if (m_window)
    {
        DestroyWindow(m_window);
        m_window = nullptr;
    }
    logfile.close();
}

LRESULT CALLBACK NewToyCOM::s_WndProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept
{
    auto thisRef = reinterpret_cast<NewToyCOM*>(GetWindowLongPtr(window, GWLP_USERDATA));
    if (!thisRef && (message == WM_CREATE))
    {
        const auto createStruct = reinterpret_cast<LPCREATESTRUCT>(lparam);
        thisRef = reinterpret_cast<NewToyCOM*>(createStruct->lpCreateParams);
        SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(thisRef));
        DefWindowProc(window, message, wparam, lparam);
    }
    return thisRef ? thisRef->WndProc(window, message, wparam, lparam) : DefWindowProc(window, message, wparam, lparam);
}

LRESULT NewToyCOM::WndProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept
{
    switch (message)
    {
    case WM_CLOSE: {
        // Don't destroy - hide instead
        isWindowShown = false;
        ShowWindow(window, SW_HIDE);
        return 0;
    }
    break;
    case WM_HOTKEY: {
        if (wparam == 1)
        {
            // Show the window if it is hidden
            if (!isWindowShown)
                ShowWindow(window, SW_SHOW);
            // Hide the window if it is shown
            else
                ShowWindow(window, SW_HIDE);
            // Toggle the state of isWindowShown
            isWindowShown = !isWindowShown;
            return 0;
        }
        else if (wparam == 2)
        {
            windowText = L"Hello World, check out this awesome power toy!";
            titleText = L"Awesome Toy";
            // Change title
            SetWindowText(window, titleText);
            // If window is shown, trigger a repaint
            RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE);
            return 0;
        }
    }
    break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(window, &ps);
        TextOut(hdc,
                // Location of the text
                10,
                10,
                // Text to print
                windowText,
                // Size of the text, my function gets this for us
                lstrlen(windowText));
        EndPaint(window, &ps);
    }
    break;
    default: {
        return DefWindowProc(window, message, wparam, lparam);
    }
    break;
    }
    return 0;
}

void createKeyEvent(WORD key_code, bool isRelease, INPUT& key_event, DWORD time = 0)
{
    key_event.type = INPUT_KEYBOARD;
    key_event.ki.wVk = key_code;
    key_event.ki.dwFlags = (isRelease ? KEYEVENTF_KEYUP : 0);
    key_event.ki.time = time;
    key_event.ki.dwExtraInfo = 0;
}

IFACEMETHODIMP_(bool)
NewToyCOM::OnKeyDown(PKBDLLHOOKSTRUCT info, WPARAM keystate) noexcept
{
    //auto startTime = std::chrono::high_resolution_clock::now();
    bool const win = GetAsyncKeyState(VK_LWIN) & 0x8000;
    bool const ctrl = GetAsyncKeyState(VK_CONTROL) & 0x8000;
    bool const alt = GetAsyncKeyState(VK_MENU) & 0x8000;
    bool const shift = GetAsyncKeyState(VK_SHIFT) & 0x8000;
    // If toggled, swap two macros
    if (m_settings->swapMacro)
    {
        if (win == m_settings->macro_first_object.win_pressed())
        {
            if (ctrl == m_settings->macro_first_object.ctrl_pressed())
            {
                if (alt == m_settings->macro_first_object.alt_pressed())
                {
                    if (shift == m_settings->macro_first_object.shift_pressed())
                    {
                        if (info->vkCode == m_settings->macro_first_object.get_code())
                        {
                            if (isSwapTriggered)
                            {
                                isSwapTriggered = false;

                                //auto stopTime = std::chrono::high_resolution_clock::now();
                                //if (logfile.is_open())
                                //{
                                //    auto duration = duration_cast<std::chrono::microseconds>(stopTime - startTime);
                                //    swapTime += duration.count();
                                //    logfile << "Macro1 - False " << duration.count() << std::endl;
                                //    logfile << "Swap Total " << swapTime << " " << swapCount << std::endl;
                                //}
                            }
                            else
                            {
                                // Assuming 1 key and modifiers only
                                int key_count = 1 + int(m_settings->macro_second_object.win_pressed()) + int(m_settings->macro_second_object.ctrl_pressed()) + int(m_settings->macro_second_object.alt_pressed()) + int(m_settings->macro_second_object.shift_pressed());
                                LPINPUT keyEventList = new INPUT[size_t(key_count)]();
                                memset(keyEventList, 0, sizeof(keyEventList));
                                int i = 0;
                                if (keystate == WM_KEYDOWN || keystate == WM_SYSKEYDOWN)
                                {
                                    if (m_settings->macro_second_object.win_pressed())
                                    {
                                        createKeyEvent(VK_LWIN, false, keyEventList[i]);
                                        ++i;
                                    }
                                    if (m_settings->macro_second_object.ctrl_pressed())
                                    {
                                        createKeyEvent(VK_CONTROL, false, keyEventList[i]);
                                        ++i;
                                    }
                                    if (m_settings->macro_second_object.alt_pressed())
                                    {
                                        createKeyEvent(VK_MENU, false, keyEventList[i]);
                                        ++i;
                                    }
                                    if (m_settings->macro_second_object.shift_pressed())
                                    {
                                        createKeyEvent(VK_SHIFT, false, keyEventList[i]);
                                        ++i;
                                    }
                                    createKeyEvent(m_settings->macro_second_object.get_code(), false, keyEventList[i]);
                                    ++i;
                                    
                                }
                                if (keystate == WM_KEYUP || keystate == WM_SYSKEYUP)
                                {
                                    createKeyEvent(m_settings->macro_second_object.get_code(), true, keyEventList[i]);
                                    ++i;
                                    if (m_settings->macro_second_object.shift_pressed())
                                    {
                                        createKeyEvent(VK_SHIFT, true, keyEventList[i]);
                                        ++i;
                                    }
                                    if (m_settings->macro_second_object.alt_pressed())
                                    {
                                        createKeyEvent(VK_MENU, true, keyEventList[i]);
                                        ++i;
                                    }
                                    if (m_settings->macro_second_object.ctrl_pressed())
                                    {
                                        createKeyEvent(VK_CONTROL, true, keyEventList[i]);
                                        ++i;
                                    }
                                    if (m_settings->macro_second_object.win_pressed())
                                    {
                                        createKeyEvent(VK_LWIN, true, keyEventList[i]);
                                        ++i;
                                    }
                                }
                                isSwapTriggered = true;
                                UINT res = SendInput(key_count, keyEventList, sizeof(INPUT));

                                // deallocation
                                delete[] keyEventList;
                                keyEventList = nullptr;

                                //auto stopTime = std::chrono::high_resolution_clock::now();
                                //if (logfile.is_open())
                                //{
                                //    auto duration = duration_cast<std::chrono::microseconds>(stopTime - startTime);
                                //    swapCount += 1;
                                //    swapTime += duration.count();
                                //    logfile << "Macro1 - True " << duration.count() << std::endl;
                                //    logfile << "Swap Total " << swapTime << " " << swapCount << std::endl;
                                //}
                                return true;
                            }
                        }
                    }
                }
            }
        }
        if (win == m_settings->macro_second_object.win_pressed())
        {
            if (ctrl == m_settings->macro_second_object.ctrl_pressed())
            {
                if (alt == m_settings->macro_second_object.alt_pressed())
                {
                    if (shift == m_settings->macro_second_object.shift_pressed())
                    {
                        if (info->vkCode == m_settings->macro_second_object.get_code())
                        {
                            if (isSwapTriggered)
                            {
                                isSwapTriggered = false;

                                //auto stopTime = std::chrono::high_resolution_clock::now();
                                //if (logfile.is_open())
                                //{
                                //    auto duration = duration_cast<std::chrono::microseconds>(stopTime - startTime);
                                //    swapTime += duration.count();
                                //    logfile << "Macro2 - False " << duration.count() << std::endl;
                                //    logfile << "Swap Total " << swapTime << " " << swapCount << std::endl;
                                //}
                            }
                            else
                            {
                                // Assuming 1 key and modifiers only
                                int key_count = 1 + int(m_settings->macro_first_object.win_pressed()) + int(m_settings->macro_first_object.ctrl_pressed()) + int(m_settings->macro_first_object.alt_pressed()) + int(m_settings->macro_first_object.shift_pressed());
                                LPINPUT keyEventList = new INPUT[size_t(key_count)]();
                                memset(keyEventList, 0, sizeof(keyEventList));
                                int i = 0;
                                if (keystate == WM_KEYDOWN || keystate == WM_SYSKEYDOWN)
                                {
                                    if (m_settings->macro_first_object.win_pressed())
                                    {
                                        createKeyEvent(VK_LWIN, false, keyEventList[i]);
                                        ++i;
                                    }
                                    if (m_settings->macro_first_object.ctrl_pressed())
                                    {
                                        createKeyEvent(VK_CONTROL, false, keyEventList[i]);
                                        ++i;
                                    }
                                    if (m_settings->macro_first_object.alt_pressed())
                                    {
                                        createKeyEvent(VK_MENU, false, keyEventList[i]);
                                        ++i;
                                    }
                                    if (m_settings->macro_first_object.shift_pressed())
                                    {
                                        createKeyEvent(VK_SHIFT, false, keyEventList[i]);
                                        ++i;
                                    }
                                    createKeyEvent(m_settings->macro_first_object.get_code(), false, keyEventList[i]);
                                    ++i;
                                }
                                if (keystate == WM_KEYUP || keystate == WM_SYSKEYUP)
                                {
                                    createKeyEvent(m_settings->macro_first_object.get_code(), true, keyEventList[i]);
                                    ++i;
                                    if (m_settings->macro_first_object.shift_pressed())
                                    {
                                        createKeyEvent(VK_SHIFT, true, keyEventList[i]);
                                        ++i;
                                    }
                                    if (m_settings->macro_first_object.alt_pressed())
                                    {
                                        createKeyEvent(VK_MENU, true, keyEventList[i]);
                                        ++i;
                                    }
                                    if (m_settings->macro_first_object.ctrl_pressed())
                                    {
                                        createKeyEvent(VK_CONTROL, true, keyEventList[i]);
                                        ++i;
                                    }
                                    if (m_settings->macro_first_object.win_pressed())
                                    {
                                        createKeyEvent(VK_LWIN, true, keyEventList[i]);
                                        ++i;
                                    }
                                }
                                isSwapTriggered = true;
                                UINT res = SendInput(key_count, keyEventList, sizeof(INPUT));

                                // deallocation
                                delete[] keyEventList;
                                keyEventList = nullptr;

                                //auto stopTime = std::chrono::high_resolution_clock::now();
                                //if (logfile.is_open())
                                //{
                                //    auto duration = duration_cast<std::chrono::microseconds>(stopTime - startTime);
                                //    swapCount += 1;
                                //    swapTime += duration.count();
                                //    logfile << "Macro2 - True " << duration.count() << std::endl;
                                //    logfile << "Swap Total " << swapTime << " " << swapCount << std::endl;
                                //}
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }

    if (keystate == WM_KEYDOWN || keystate == WM_SYSKEYDOWN)
    {
        // Note: Win+L cannot be overriden. Requires WinLock to be disabled.
        // Trigger on Win+Z
        if (win == m_settings->newToyLLHotkeyObject.win_pressed())
        {
            if (ctrl == m_settings->newToyLLHotkeyObject.ctrl_pressed())
            {
                if (alt == m_settings->newToyLLHotkeyObject.alt_pressed())
                {
                    if (shift == m_settings->newToyLLHotkeyObject.shift_pressed())
                    {
                        if (info->vkCode == m_settings->newToyLLHotkeyObject.get_code())
                        {
                            if (m_window)
                            {
                                // Show the window if it is hidden
                                if (!isWindowShown)
                                    ShowWindow(m_window, SW_SHOW);
                                // Hide the window if it is shown
                                else
                                    ShowWindow(m_window, SW_HIDE);
                                // Toggle the state of isWindowShown
                                isWindowShown = !isWindowShown;
                                // Return true to swallow the keyboard event
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }

    // If toggled, replace Win+R with Win+S
    if (m_settings->swapWRS)
    {
        bool keyR = GetAsyncKeyState('R') & 0x8000;
        if (info->vkCode == VK_LWIN && keystate == WM_KEYUP && keyR)
        {
            if (winSflag)
            {
                LPINPUT keyEventList = new INPUT[1]();
                memset(keyEventList, 0, sizeof(keyEventList));
                createKeyEvent('S', true, keyEventList[0]);
                UINT res = SendInput(1, keyEventList, sizeof(INPUT));

                // deallocation
                delete[] keyEventList;
                keyEventList = nullptr;
                winSflag = false;
                //auto stopTime = std::chrono::high_resolution_clock::now();
                //if (logfile.is_open())
                //{
                //    auto duration = duration_cast<std::chrono::microseconds>(stopTime - startTime);
                //    logfile << "WinR false " << duration.count() << std::endl;
                //}
                return true;
            }
        }
        if (win && info->vkCode == 'R')
        {
            // allocation
            LPINPUT keyEventList = new INPUT[2]();
            memset(keyEventList, 0, sizeof(keyEventList));

            if (keystate == WM_KEYDOWN || keystate == WM_SYSKEYDOWN)
            {
                createKeyEvent(VK_LWIN, false, keyEventList[0]);
                createKeyEvent('S', false, keyEventList[1]);
                winSflag = true;
                //winRcount += 1;
            }
            else if (keystate == WM_KEYUP || keystate == WM_SYSKEYUP)
            {
                createKeyEvent('S', true, keyEventList[0]);
                //createKeyEvent(VK_LWIN, true, keyEventList[1]);
                winSflag = false;
            }

            SendInput(2, keyEventList, sizeof(INPUT));
            // deallocation
            delete[] keyEventList;
            keyEventList = nullptr;
            //auto stopTime = std::chrono::high_resolution_clock::now();
            //if (logfile.is_open())
            //{
            //    auto duration = duration_cast<std::chrono::microseconds>(stopTime - startTime);
            //    winRtime += duration.count();
            //    logfile << "WinR true " << duration.count() << std::endl;
            //    logfile << "WinR Total " << winRtime << " " << winRcount << std::endl;
            //}
            return true;
        }
    }

    return false;
}

IFACEMETHODIMP_(void)
NewToyCOM::HotkeyChanged() noexcept
{
    // Update the hotkey
    UnregisterHotKey(m_window, 1);
    RegisterHotKey(m_window, 1, m_settings->newToyShowHotkey.get_modifiers(), m_settings->newToyShowHotkey.get_code());
    UnregisterHotKey(m_window, 2);
    RegisterHotKey(m_window, 2, m_settings->newToyEditHotkey.get_modifiers(), m_settings->newToyEditHotkey.get_code());
}

winrt::com_ptr<INewToy> MakeNewToy(HINSTANCE hinstance, ModuleSettings* settings) noexcept
{
    return winrt::make_self<NewToyCOM>(hinstance, settings);
}