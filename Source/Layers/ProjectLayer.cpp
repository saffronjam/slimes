#include "ProjectLayer.h"

namespace Se
{
void ProjectLayer::OnAttach(std::shared_ptr<Batch>& batch)
{
	BaseLayer::OnAttach(batch);
	batch->Submit([this]
		{
		}, "Create Simulation Manager");
	_simMgr = std::make_shared<SimulationManager>();
}

void ProjectLayer::OnDetach()
{
	BaseLayer::OnDetach();
}

void ProjectLayer::OnUpdate()
{
	BaseLayer::OnUpdate();
	_simMgr->OnUpdate(_scene);
	_simMgr->OnRender(_scene);
}

void ProjectLayer::OnGuiRender()
{
	BaseLayer::OnGuiRender();
	
	if ( ImGui::Begin("Project") )
	{
		_simMgr->OnGuiRender();
	}
	ImGui::End();
}

void ProjectLayer::OnRenderTargetResize(const sf::Vector2f &newSize)
{
	BaseLayer::OnRenderTargetResize(newSize);
	_scene.OnRenderTargetResize(newSize);
}
}
