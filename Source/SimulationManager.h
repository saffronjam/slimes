#pragma once

#include <SFML/Graphics.hpp>
#include <Saffron.h>

#include "Agent.h"

namespace Se
{
class SimulationManager
{
	enum class ShapeType
	{
		Circle,
		Square,
		Random
	};

	enum class AngleType
	{
		CenterIn,
		CenterOut,
		Random
	};

	enum class PaletteType
	{
		Slime,
		Fiery,
		Greyscale,
		Rainbow,
		UV,
		Count
	};

	enum class QualityType
	{
		Low,
		Medium,
		High
	};

	enum class StateType
	{
		Paused,
		Running
	};

public:
	explicit SimulationManager(QualityType initialQuality = QualityType::Medium);

	void OnUpdate(Scene& scene);
	void OnRender(Scene& scene);
	void OnGuiRender();

private:
	void UpdatePaletteTransition();
	void UpdatePaletteData();

	auto PositionGenerator(ShapeType shapeType) -> std::function<sf::Vector2f(int)>;
	auto AngleGenerator(AngleType angleType) -> std::function<float(int)>;

	void SetShape(const std::function<sf::Vector2f(int)>& generator);
	void SetAngles(const std::function<float(int)>& generator);
	void SetPalette(PaletteType desired);
	void SetQuality(QualityType quality);
	void ResizeTexture(uint width, uint height);
	void ResizeAgentDimension(uint size);

	void Reset(ShapeType shapeType, AngleType angleType);
	void Transition(ShapeType shapeTypeTo, AngleType angleTypeTo);

	void RunDrawFrame();

	// Fix for problem with using OpenGL freely alongside SFML
	static void SetUniform(uint id, const std::string& name, const sf::Vector2<double>& value);
	static void SetUniform(uint id, const std::string& name, float value);
	static void SetUniform(uint id, const std::string& name, double value);
	static void SetUniform(uint id, const std::string& name, int value);

private:
	static constexpr uint _paletteWidth = 2048;

	StateType _stateType = StateType::Paused;

	uint _agentDim;
	uint _texWidth;
	uint _texHeight;

	std::vector<Agent> _agentBuffer;

	sf::Image _simulationImage;

	sf::Texture _dataTexture;
	sf::RenderTexture _targetTexture;

	std::shared_ptr<ComputeShader> _drawCS;
	std::shared_ptr<sf::Shader> _painterPS;
	std::shared_ptr<sf::Shader> _blendEvapPaintPS;
	uint _ssbo;

	ShapeType _shapeType = ShapeType::Circle;
	AngleType _angleType = AngleType::CenterIn;

	std::array<std::shared_ptr<sf::Image>, static_cast<size_t>(PaletteType::Count)> _paletteImages;
	std::array<sf::Texture, static_cast<size_t>(PaletteType::Count)> _palettes;
	sf::Image _currentPaletteImage;
	sf::Texture _currentPaletteTexture;
	PaletteType _desiredPalette = PaletteType::Slime;
	std::array<sf::Vector4f, _paletteWidth> _colorsStart;
	std::array<sf::Vector4f, _paletteWidth> _colorsCurrent;

	sf::Time _colorTransitionTimer;
	sf::Time _colorTransitionDuration = sf::seconds(0.3f);
	bool _inTransition = false;

	// Gui cache
	float _movementSpeed = 100.0f;
	float _trailAttraction = 100.0f;
	float _diffuseSpeed = 5.0f;
	float _evaporateSpeed = 6.5f;
	float _colorScale = 5.0f;

	std::vector<const char*> _shapeTypeNames;
	std::vector<const char*> _angleTypeNames;
	std::vector<const char*> _paletteTypeNames;
	std::vector<const char*> _qualityTypeNames;
	int _shapeTypeIndex = static_cast<int>(_shapeType);
	int _angleTypeIndex = static_cast<int>(_angleType);
	int _paletteTypeIndex = static_cast<int>(_desiredPalette);
	int _qualityTypeIndex;
};
}
