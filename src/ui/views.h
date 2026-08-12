#pragma once

class App;

namespace Views {

void RenderConnect(App& app);   // matches the reference screenshot exactly
void RenderDevices(App& app);
void RenderControl(App& app);
void RenderApps(App& app);
void RenderFiles(App& app);
void RenderShell(App& app);
void RenderSettings(App& app);
void RenderAbout(App& app);

} // namespace Views
