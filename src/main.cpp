/*
 * Copyright (c) 2021 Soar Qin<soarchin@gmail.com>
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "cfg.h"

#include "renderer.h"
#include "window.h"
#include "maprenderer.h"
#include "util.h"
#include "os_structs.h"

#include "d2map.h"

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    osInit();
    HANDLE evt = CreateEventW(nullptr, FALSE, FALSE, L"Global\\D2RMH_EVENT");
    if (!evt) {
        return -1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(evt);
        MessageBoxW(nullptr, L"A D2RMH instance is already running!", L"D2RMH", MB_OK | MB_ICONERROR);
        return 0;
    }
    loadCfg();
    d2mapapi::PipedChildProcess pcp;
    if (!pcp.start(L"d2mapapi_piped.exe", (wchar_t *)cfg->d2Path.c_str())) {
        MessageBoxA(nullptr, pcp.errMsg().c_str(), nullptr, 0);
        return -1;
    }
    loadData();

    Window wnd(100, 100, 500, 400);
    Renderer renderer(&wnd);
    if (cfg->fps > 0) {
        renderer.limitFPS(cfg->fps);
    } else {
        Renderer::setSwapInterval(-cfg->fps);
    }

    MapRenderer map(renderer, pcp);
    wnd.enableTrayMenu(true,
                       (const wchar_t *)1,
                       L"D2RMH " VERSION_STRING_FULL,
                       L"D2RMH is running.\nYou can close it from tray-icon popup menu.",
                       L"D2RMH " VERSION_STRING);
    wnd.addAboutMenu();
    wnd.addTrayMenuItem(L"Reload Config", -1, 0, [&wnd, &renderer, &map]() {
        loadCfg();
        wnd.reloadConfig();
        if (cfg->fps > 0) {
            renderer.limitFPS(cfg->fps);
        } else {
            Renderer::setSwapInterval(-cfg->fps);
        }
        map.reloadConfig();
    });
    wnd.addTrayMenuItem(L"Quit", -1, 0, [&wnd]() { wnd.quit(); });
    while (wnd.run()) {
        updateTime();
        renderer.prepare();
        map.update();
        renderer.begin();
        map.render();
        renderer.end();
        if (cfg->renderDelay > 0) {
            Sleep(cfg->renderDelay);
        }
    }
    return 0;
}
