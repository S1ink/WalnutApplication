#include "Renderer.h"

#include "Walnut/Random.h"


template<typename t>
t clamp(t n, t h, t l) {
	return n >= h ? h : n <= l ? l : n;
}

void Renderer::resize(uint32_t w, uint32_t h) {
	if (this->image) {
		if (this->image->GetWidth() == w && this->image->GetHeight() == h) {
			return;
		}
		this->image->Resize(w, h);
	}
	else {
		this->image = std::make_shared<Walnut::Image>(w, h, Walnut::ImageFormat::RGBA);
	}

	delete[] this->buffer;
	this->buffer = new uint32_t[w * h];
}
void Renderer::render() {
	for (uint32_t y = 0; y < this->image->GetHeight(); y++) {
		for (uint32_t x = 0; x < this->image->GetWidth(); x++) {

			this->buffer[ x + y * this->image->GetWidth() ] =
				this->imFunc( { 2 * x / (float)this->image->GetWidth() - 1.f, 2 * y / (float)this->image->GetHeight() - 1.f } );
	
		}
	}
	this->image->SetData(this->buffer);
}
uint32_t Renderer::imFunc(glm::vec2 coord) {

	glm::vec3
		d = glm::normalize(glm::vec3{coord, -1.f}),
		o = this->origin
	;
	float
		rad = 0.5f,
		a = glm::dot(d, d),
		b = 2.f * glm::dot(o, d),
		c = glm::dot(o, o) - (rad * rad),
		disc = (b * b) - 4.f * a * c,
		q1 = (sqrtf(disc) - b) / (2 * a),
		q2 = (sqrtf(disc) + b) / (2 * a),
		q = abs(q1) < abs(q2) ? q1 : q2
	;

	if (disc >= 0.f) {
		return 0xff000000 | (uint8_t)clamp<int>(255 - (abs(q) * this->brightness), 255, 0) << 8;
	}
	else {
		return 0xff000000;
	}
}