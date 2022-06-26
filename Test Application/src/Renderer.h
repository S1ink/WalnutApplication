#pragma once

#include "Walnut/Image.h"
#include <memory>
#include <glm/glm.hpp>


class Renderer {
public:
	Renderer() = default;

	void resize(uint32_t, uint32_t);
	void render();
	inline void moveOrigin(glm::vec3 v) { this->origin += v; }
	inline void moveBrightness(float v) { this->brightness += v; }

	inline std::shared_ptr<Walnut::Image> getOutput() const { return this->image; }

protected:
	uint32_t imFunc(glm::vec2 coord);

private:
	std::shared_ptr<Walnut::Image> image;
	uint32_t* buffer = nullptr;
	glm::vec3
		origin{0.f, 0.f, 1.f}
	;
	float brightness = 100.f;


};