#include <memory>
#define SAFFRON_ENTRY_POINT
#include "project_app.h"

namespace saffron
{
auto CreateApplication() -> std::unique_ptr<App>
{
	return std::make_unique<slimes::ProjectApp>(AppProperties::CreateFullscreen("slimes"));
}
}

namespace slimes
{
using namespace saffron;
ProjectApp::ProjectApp(const AppProperties& properties) :
	App(properties),
	_projectLayer(std::make_shared<ProjectLayer>())
{
}

void ProjectApp::OnInit()
{
	PushLayer(_projectLayer);
}

void ProjectApp::OnUpdate()
{
	App::OnUpdate();
}
}
