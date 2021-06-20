#include "SaffronPCH.h"

#include <glad/glad.h>

#include "SimulationManager.h"

namespace Se
{
SimulationManager::SimulationManager(QualityType initialQuality) :
	_shapeTypeNames({"Circle", "Square", "Random"}),
	_angleTypeNames({"Center In", "Center Out", "Random"}),
	_paletteTypeNames({"Slime", "Fiery", "Greyscale", "Rainbow", "UV"}),
	_qualityTypeNames({"Low", "Medium", "High"})
{
	_drawCS = ComputeShaderStore::Get("draw.comp");
	_blendEvapPaintPS = ShaderStore::Get("blendEvapPaint.frag", sf::Shader::Type::Fragment);
	_painterPS = ShaderStore::Get("painter.frag", sf::Shader::Type::Fragment);

	// Palette read-textures
	const String filepaths[] = {"slimeLarge.png", "fieryLarge.png", "greyscaleLarge.png", "rainbowLarge.png", "uvLarge.png"};

	for (int i = 0; i < _palettes.size(); i++)
	{
		auto image = ImageStore::Get("Pals/" + filepaths[i]);
		Debug::Assert(image->getSize().x == _paletteWidth && image->getSize().y == 1);
		_paletteImages[i] = image;

		_palettes[i].loadFromImage(*image);
		const auto size = _palettes[i].getSize();

		glBindTexture(GL_TEXTURE_2D, _palettes[i].getNativeHandle());
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->getPixelsPtr());
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	// SSBO
	SetQuality(initialQuality);
	_qualityTypeIndex = static_cast<int>(initialQuality);
	glGenBuffers(1, &_ssbo);

	// Palette-transitions
	_currentPaletteImage.create(_paletteWidth, 1, _paletteImages[static_cast<int>(_desiredPalette)]->getPixelsPtr());
	_currentPaletteTexture.loadFromImage(_currentPaletteImage);

	for (int i = 0; i < _paletteWidth; i++)
	{
		const auto pix = _currentPaletteImage.getPixel(i, 0);
		_colorsStart[i] = {
			static_cast<float>(pix.r) / 255.0f, static_cast<float>(pix.g) / 255.0f, static_cast<float>(pix.b) / 255.0f,
			static_cast<float>(pix.a) / 255.0f
		};
	}
	_colorsCurrent = _colorsStart;

	Transition(_shapeType, _angleType);
}

void SimulationManager::OnUpdate(Scene& scene)
{
	UpdatePaletteTransition();
	UpdatePaletteData();

	glBindImageTexture(1, _dataTexture.getNativeHandle(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(2, _currentPaletteTexture.getNativeHandle(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	_targetTexture.clear();
	if (_stateType != StateType::Paused)
	{
		const float dt = Global::Clock::FrameTime().asSeconds();

		if (!_inTransition)
		{
			RunDrawFrame();
		}

		SetUniform(_blendEvapPaintPS->getNativeHandle(), "dt", dt);
		SetUniform(_blendEvapPaintPS->getNativeHandle(), "diffuseSpeed", _inTransition ? 40.0f : _diffuseSpeed);
		SetUniform(_blendEvapPaintPS->getNativeHandle(), "evaporateSpeed", _inTransition ? 80.0f : _evaporateSpeed);
		SetUniform(_blendEvapPaintPS->getNativeHandle(), "blendSize", 3);
		SetUniform(_blendEvapPaintPS->getNativeHandle(), "maxPixelValue", _colorScale);
		SetUniform(_blendEvapPaintPS->getNativeHandle(), "paletteWidth", static_cast<int>(_paletteWidth));

		sf::RectangleShape simRectShape(sf::Vector2f(_texWidth, _texHeight));
		simRectShape.setTexture(&_currentPaletteTexture);
		_targetTexture.draw(simRectShape, {_blendEvapPaintPS.get()});
	}
	else
	{
		SetUniform(_painterPS->getNativeHandle(), "maxPixelValue", _colorScale);
		SetUniform(_painterPS->getNativeHandle(), "paletteWidth", static_cast<int>(_paletteWidth));

		sf::RectangleShape simRectShape(sf::Vector2f(_texWidth, _texHeight));
		simRectShape.setTexture(&_currentPaletteTexture);
		_targetTexture.draw(simRectShape, {_painterPS.get()});
	}
	_targetTexture.display();

	App::Instance().Window().NativeWindow().resetGLStates();
}

void SimulationManager::OnRender(Scene& scene)
{
	const auto size = scene.ViewportPane().ViewportSize();
	const auto diff = sf::Vector2f(size.x / _texWidth, size.y / _texHeight);

	sf::Sprite sprite(_targetTexture.getTexture());
	sprite.setPosition(-size.x / 2.0f, -size.y / 2.0f);
	sprite.setScale(diff.x, diff.y);
	scene.Submit(sprite);
}

void SimulationManager::OnGuiRender()
{
	if (ImGui::Button(_stateType == StateType::Paused ? "Start" : "Pause", {ImGui::GetContentRegionAvailWidth(), 0.0f}))
	{
		_stateType = _stateType == StateType::Paused ? StateType::Running : StateType::Paused;
	}

	ImGui::Separator();

	Gui::BeginPropertyGrid("Simulation Manager");
	Gui::Property("Movement Speed", _movementSpeed, 10.0f, 500.0f, 1.0f, GuiPropertyFlag_Slider);
	Gui::Property("Trail Attraction", _trailAttraction, 10.0f, 500.0f, 1.0f, GuiPropertyFlag_Slider);
	Gui::Property("Diffuse Speed", _diffuseSpeed, 0.0f, 25.0f, 0.01f, GuiPropertyFlag_Slider);
	Gui::Property("Evaporate Speed", _evaporateSpeed, 0.0, 25.0f, 0.01f, GuiPropertyFlag_Slider);
	Gui::Property("Color Scale", _colorScale, 1.0, 25.0f, 0.5f, GuiPropertyFlag_Slider);
	Gui::EndPropertyGrid();

	ImGui::Separator();

	Gui::BeginPropertyGrid("Reset Patterns");
	ImGui::Text("Reset Shape");
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);
	ImGui::Combo("##Shape Type", &_shapeTypeIndex, _shapeTypeNames.data(), _shapeTypeNames.size());
	ImGui::NextColumn();
	ImGui::Text("Reset Angle");
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);
	ImGui::Combo("##Angle Type", &_angleTypeIndex, _angleTypeNames.data(), _angleTypeNames.size());
	ImGui::NextColumn();
	Gui::Property("Reset", [this]
	{
		Transition(static_cast<ShapeType>(_shapeTypeIndex), static_cast<AngleType>(_angleTypeIndex));
	});
	Gui::EndPropertyGrid();

	ImGui::Separator();

	Gui::BeginPropertyGrid("Quality");
	ImGui::Text("Quality");
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);
	if (ImGui::Combo("##Quality", &_qualityTypeIndex, _qualityTypeNames.data(), _qualityTypeNames.size()))
	{
		SetQuality(static_cast<QualityType>(_qualityTypeIndex));
	}
	ImGui::NextColumn();
	Gui::Property("Agents", std::to_string(_agentDim * _agentDim));
	Gui::Property("Texture Size", std::to_string(_texWidth) + "x" + std::to_string(_texHeight) + " px");
	Gui::EndPropertyGrid();

	ImGui::Separator();

	Gui::BeginPropertyGrid("Palette");
	ImGui::Text("Palette");
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);
	if (ImGui::Combo("##Palette", &_paletteTypeIndex, _paletteTypeNames.data(), _paletteTypeNames.size()))
	{
		SetPalette(static_cast<PaletteType>(_paletteTypeIndex));
	}
	ImGui::NextColumn();
	Gui::EndPropertyGrid();
	ImGui::Dummy({1.0f, 2.0f});
	Gui::Image(_currentPaletteTexture, sf::Vector2f(ImGui::GetContentRegionAvailWidth(), 9.0f));
}

void SimulationManager::UpdatePaletteTransition()
{
	if (_colorTransitionTimer <= _colorTransitionDuration)
	{
		const float delta = (std::sin((_colorTransitionTimer / _colorTransitionDuration) * PI<> - PI<> / 2.0f) + 1.0f) /
			2.0f;
		for (int x = 0; x < _paletteWidth; x++)
		{
			const auto pix = _paletteImages[static_cast<int>(_desiredPalette)]->getPixel(x, 0);
			const sf::Vector4f goalColor = {
				static_cast<float>(pix.r) / 255.0f, static_cast<float>(pix.g) / 255.0f,
				static_cast<float>(pix.b) / 255.0f, static_cast<float>(pix.a) / 255.0f
			};
			const auto& startColor = _colorsStart[x];
			auto& currentColor = _colorsCurrent[x];
			currentColor.x = startColor.x + delta * (goalColor.x - startColor.x);
			currentColor.y = startColor.y + delta * (goalColor.y - startColor.y);
			currentColor.z = startColor.z + delta * (goalColor.z - startColor.z);
			_currentPaletteImage.setPixel(x, 0, {
				                              static_cast<sf::Uint8>(currentColor.x * 255.0f),
				                              static_cast<sf::Uint8>(currentColor.y * 255.0f),
				                              static_cast<sf::Uint8>(currentColor.z * 255.0f),
				                              static_cast<sf::Uint8>(currentColor.w * 255.0f)
			                              });
		}
		_colorTransitionTimer += Global::Clock::FrameTime();
	}
}

void SimulationManager::UpdatePaletteData()
{
	glBindTexture(GL_TEXTURE_2D, _currentPaletteTexture.getNativeHandle());
	const auto size = _currentPaletteTexture.getSize();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE,
	             _currentPaletteImage.getPixelsPtr());
	glBindTexture(GL_TEXTURE_2D, 0);
}

Function<sf::Vector2f(int)> SimulationManager::PositionGenerator(ShapeType shapeType)
{
	switch (_shapeType)
	{
	case ShapeType::Circle:
	{
		return [this](int index)
		{
			const float relative = static_cast<float>(index) / static_cast<float>(_agentBuffer.size()) * 2.0f *
				Math::PI;

			const float multiplier = Random::Real() * (_texHeight / 3.0f - 50.0f);
			const auto offset = sf::Vector2f(_texWidth, _texHeight) / 2.0f;
			return sf::Vector2f(std::cos(relative), std::sin(relative)) * multiplier + offset;
		};
	}
	case ShapeType::Square:
	{
		return [this](int index)
		{
			const sf::Vector2f toMiddle = sf::Vector2f(_texWidth, _texHeight) / 2.0f;
			const float randomRange = (std::min(_texWidth, _texHeight) / 2.5f) / 2.0f;

			return toMiddle + Random::Vec2(-randomRange, -randomRange, randomRange, randomRange);
		};
	}
	case ShapeType::Random:
	{
		return [this](int index)
		{
			return Random::Vec2(VecUtils::Null<>(), sf::Vector2f(_texWidth, _texHeight));
		};
	}
	default:
	{
		Debug::Break("Invalid Shape Type");
		return {};
	}
	}
}

Function<float(int)> SimulationManager::AngleGenerator(AngleType angleType)
{
	switch (_angleType)
	{
	case AngleType::CenterIn:
	{
		return [this](int index)
		{
			return 180.0f + static_cast<float>(index) / static_cast<float>(_agentBuffer.size()) * 2.0f * Math::PI;
		};
	}
	case AngleType::CenterOut:
	{
		return [this](int index)
		{
			return static_cast<float>(index) / static_cast<float>(_agentBuffer.size()) * 2.0f * Math::PI;
		};
	}
	case AngleType::Random:
	{
		return [this](int index)
		{
			return Random::Real() * 2.0f * Math::PI;
		};
	}
	default: Debug::Break("Invalid Angle Type");
		return {};
	}
}

void SimulationManager::SetShape(const Function<sf::Vector2f(int)>& generator)
{
	for (int i = 0; i < _agentBuffer.size(); i++)
	{
		_agentBuffer[i].Position = generator(i);
	}
}

void SimulationManager::SetAngles(const Function<float(int)>& generator)
{
	for (int i = 0; i < _agentBuffer.size(); i++)
	{
		_agentBuffer[i].Angle = generator(i);
	}
}

void SimulationManager::SetPalette(PaletteType desired)
{
	_desiredPalette = desired;
	_colorTransitionTimer = sf::Time::Zero;
	_colorsStart = _colorsCurrent;
}

void SimulationManager::SetQuality(QualityType quality)
{
	switch (quality)
	{
	case QualityType::Low:
	{
		ResizeTexture(700, 400);
		ResizeAgentDimension(128);
		break;
	}
	case QualityType::Medium:
	{
		ResizeTexture(1400, 800);
		ResizeAgentDimension(512);
		break;
	}
	case QualityType::High:
	{
		ResizeTexture(2100, 1200);
		ResizeAgentDimension(1024);
		break;
	}
	default:
	{
		Debug::Break("Invalid Quality Type");
	}
	}
	Transition(_shapeType, _angleType);
}

void SimulationManager::ResizeTexture(Uint32 width, Uint32 height)
{
	if (_texWidth == width && _texHeight == height)
	{
		return;
	}

	_texWidth = width;
	_texHeight = height;

	_dataTexture.create(_texWidth, _texHeight);
	_targetTexture.create(_texWidth, _texHeight);

	glBindTexture(GL_TEXTURE_2D, _dataTexture.getNativeHandle());
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _texWidth, _texHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
	glBindTexture(GL_TEXTURE_2D, _targetTexture.getTexture().getNativeHandle());
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _texWidth, _texHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void SimulationManager::ResizeAgentDimension(Uint32 size)
{
	_agentBuffer.resize(size * size);
	_agentDim = size;
}

void SimulationManager::Reset(ShapeType shapeType, AngleType angleType)
{
	_angleType = angleType;
	_shapeType = shapeType;

	const auto angleGenerator = AngleGenerator(angleType);
	const auto positionGenerator = PositionGenerator(shapeType);

	SetAngles(angleGenerator);
	SetShape(positionGenerator);

	const auto alloc = _agentBuffer.size() * sizeof(Agent);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, alloc, _agentBuffer.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	RunDrawFrame();

	ComputeShader::AwaitFinish();
}

void SimulationManager::Transition(ShapeType shapeTypeTo, AngleType angleTypeTo)
{
	if (!_inTransition)
	{
		_stateType = StateType::Running;
		_inTransition = true;

		Run::After([this, shapeTypeTo, angleTypeTo]
		{
			_stateType = StateType::Paused;
			Reset(shapeTypeTo, angleTypeTo);
			_inTransition = false;
		}, sf::seconds(1.0f));
	}
}

void SimulationManager::RunDrawFrame()
{
	const float dt = 1.0f / 150.0f;

	glBindImageTexture(1, _dataTexture.getNativeHandle(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(2, _currentPaletteTexture.getNativeHandle(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _ssbo);

	_drawCS->SetInt("agentDim", _agentDim);
	_drawCS->SetFloat("width", _texWidth);
	_drawCS->SetFloat("height", _texHeight);
	_drawCS->SetFloat("dt", dt);
	_drawCS->SetFloat("moveSpeed", _movementSpeed);
	_drawCS->SetFloat("turnSpeed", _trailAttraction);
	_drawCS->SetFloat("sensorAngleSpacing", Math::PI / 8.0f);
	_drawCS->SetFloat("maxPixelValue", 5.0);
	_drawCS->Dispatch(_agentDim, _agentDim, 1);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void SimulationManager::SetUniform(Uint32 id, const String& name, const sf::Vector2<double>& value)
{
	glUseProgram(id);

	const auto loc = glGetUniformLocation(id, name.c_str());
	Debug::Assert(loc != -1);
	glUniform2d(loc, value.x, value.y);

	glUseProgram(0);
}

void SimulationManager::SetUniform(Uint32 id, const String& name, float value)
{
	glUseProgram(id);

	const auto loc = glGetUniformLocation(id, name.c_str());
	Debug::Assert(loc != -1);
	glUniform1f(loc, value);

	glUseProgram(0);
}

void SimulationManager::SetUniform(Uint32 id, const String& name, double value)
{
	glUseProgram(id);

	const auto loc = glGetUniformLocation(id, name.c_str());
	Debug::Assert(loc != -1);
	glUniform1d(loc, value);

	glUseProgram(0);
}

void SimulationManager::SetUniform(Uint32 id, const String& name, int value)
{
	glUseProgram(id);

	const auto loc = glGetUniformLocation(id, name.c_str());
	Debug::Assert(loc != -1);
	glUniform1i(loc, value);

	glUseProgram(0);
}
}
