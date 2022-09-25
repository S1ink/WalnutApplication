#include <chrono>

#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Image.h"
#include "Walnut/Timer.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Renderer.h"
#include "Camera.h"
#include "Scene.h"


class RenderLayer : public Walnut::Layer
{
public:
	RenderLayer() : scene{
		{
			Sphere{glm::vec3{0.f, 0.f, 0.f}, 0.5f, glm::vec3{1.f, 0.f, 1.f}},
			Sphere{glm::vec3{0.f, 0.f, -5.f}, 0.75f, glm::vec3{0.f, 1.f, 1.f}}
		}
	} {}


	virtual void OnUpdate(float ts) override {
		this->camera.OnUpdate(ts);
	}
	virtual void OnUIRender() override {

		using hrc = std::chrono::high_resolution_clock;
		static hrc::time_point ref = hrc::now();
		
		ImGui::Begin("Options"); {
			ImGui::Text("FPS: %.3f", 1e9 / (hrc::now() - ref).count());
			ref = hrc::now();
			ImGui::Separator();
			for (size_t i = 0; i < this->scene.spheres.size(); i++) {
				ImGui::PushID(i);
				if (ImGui::CollapsingHeader(("Obj " + std::to_string(i)).c_str())) {
					ImGui::DragFloat3("Position", glm::value_ptr(this->scene.spheres[i].position), 0.1);
					ImGui::DragFloat("Size", &this->scene.spheres[i].rad, 0.1);
					ImGui::ColorEdit3("Albedo", glm::value_ptr(this->scene.spheres[i].albedo));
				}
				ImGui::PopID();
			}

		} ImGui::End();

		ImGui::Begin("Render"); {
			this->frame_width = ImGui::GetContentRegionAvail().x;
			this->frame_height = ImGui::GetContentRegionAvail().y;

			if (this->frame_width * this->frame_height > 0) {
				auto frame = this->render();
				if (frame) {
					ImGui::Image(frame->GetDescriptorSet(), { (float)frame->GetWidth(), (float)frame->GetHeight() }, ImVec2(0, 1), ImVec2(1, 0));
				}
			}
		} ImGui::End();

	}
protected:
	std::shared_ptr<Walnut::Image> render() {
		this->renderer.resize(this->frame_width, this->frame_height);
		this->camera.OnResize(this->frame_width, this->frame_height);
		this->renderer.render(this->scene, this->camera);
		return this->renderer.getOutput();
	}

private:
	Renderer renderer;
	Camera camera{45.f, 0.1f, 100.f};
	Scene scene;
	uint32_t frame_width = 0, frame_height = 0;

	float ltime = 0.f;
	float lscroll = ImGui::GetIO().MouseWheel;


};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Application";
	spec.Width = 1280;
	spec.Height = 720;

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<RenderLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}