#pragma once

#include <memory>

#include "layers/base_layer.h"

#include "simulation_manager.h"

namespace slimes
{
using namespace saffron;
class ProjectLayer : public BaseLayer
{
public:
	void OnAttach(std::shared_ptr<Batch>& batch) override;
	void OnDetach() override;

	void OnUpdate() override;
	void OnGuiRender() override;

	void OnRenderTargetResize(const sf::Vector2f &newSize) override;

private:
	std::shared_ptr<SimulationManager> _simMgr;

};
}
