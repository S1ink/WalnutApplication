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
			new Sphere(glm::vec3{0.f, 0.f, 0.f}, glm::vec3{1.f, 0.f, 1.f}, 0.5f, 0.05f),
			new Sphere(glm::vec3{5.f, 0.f, 0.f}, glm::vec3{1.f, 1.f, 0.f}, 0.8f, 0.05f),
			new Sphere(glm::vec3{0.f, 0.f, -5.f}, glm::vec3{0.f, 1.f, 1.f}, 0.75f, 0.05f)/*,
			new Triangle(
				glm::vec3{100, -3, 100}, glm::vec3{-100, -3, 100}, glm::vec3{-100, -3, -100}, glm::vec3{1}
			),
			new Triangle(
				glm::vec3{100, -3, 100}, glm::vec3{100, -3, -100}, glm::vec3{-100, -3, -100}, glm::vec3{1}
			)*/
			/*new Triangle(glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0}, glm::vec3{1, 0, 0}, glm::vec3{0, 1, 0}),
			new Triangle(glm::vec3{1, 1, 0}, glm::vec3{0, 1, 0}, glm::vec3{1, 0, 0}, glm::vec3{0, 1, 0}),
			new Triangle(glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0}, glm::vec3{0, 0, 1}, glm::vec3{0, 1, 0}),
			new Triangle(glm::vec3{0, 1, 1}, glm::vec3{0, 1, 0}, glm::vec3{0, 0, 1}, glm::vec3{0, 1, 0}),
			new Triangle(glm::vec3{0, 0, 0}, glm::vec3{1, 0, 0}, glm::vec3{0, 0, 1}, glm::vec3{0, 1, 0}),
			new Triangle(glm::vec3{1, 0, 1}, glm::vec3{1, 0, 0}, glm::vec3{0, 0, 1}, glm::vec3{0, 1, 0})*/
			/*,
			new Triangle(glm::vec3{}, glm::vec3{}, glm::vec3{}, glm::vec3{0, 1, 0}),
			new Triangle(glm::vec3{}, glm::vec3{}, glm::vec3{}, glm::vec3{0, 1, 0}),
			new Triangle(glm::vec3{}, glm::vec3{}, glm::vec3{}, glm::vec3{0, 1, 0}),
			new Triangle(glm::vec3{}, glm::vec3{}, glm::vec3{}, glm::vec3{0, 1, 0}),
			new Triangle(glm::vec3{}, glm::vec3{}, glm::vec3{}, glm::vec3{0, 1, 0}),
			new Triangle(glm::vec3{}, glm::vec3{}, glm::vec3{}, glm::vec3{0, 1, 0})*/
		},
		{
			new Sphere(glm::vec3{0}, glm::vec3{1.f}),
			new Sphere(glm::vec3{-4.f, 3.f, 2.f}, glm::vec3{0.f, 1.f, 0.5f}, 1.f)/*,
			new Triangle(
				glm::vec3{4, 5, 6}, glm::vec3{7, 4, 7}, glm::vec3{6, 5, 4}, glm::vec3{0.8, 0.5, 0})*/
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
			ImGui::SameLine();
			if (ImGui::Button(this->paused ? "Unpause Render" : "Pause Render")) {
				this->paused = !this->paused;
			}
			ImGui::Separator();
			for (size_t i = 0; i < this->scene.objects.size(); i++) {
				ImGui::PushID(i);
				if (ImGui::CollapsingHeader(("Obj " + std::to_string(i)).c_str())) {
					ImGui::DragFloat3("Position", glm::value_ptr(this->scene.objects[i]->position), 0.1);
					if (Sphere* s = dynamic_cast<Sphere*>(this->scene.objects[i])) {
						ImGui::DragFloat("Size", &s->rad, 0.1);
					}
					ImGui::ColorEdit3("Albedo", glm::value_ptr(this->scene.objects[i]->albedo));
					ImGui::DragFloat("Roughness", &this->scene.objects[i]->roughness);
					ImGui::DragFloat("Matallic", &this->scene.objects[i]->metallic);
				}
				ImGui::PopID();
			}

		} ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Render"); {
			this->frame_width = ImGui::GetContentRegionAvail().x;
			this->frame_height = ImGui::GetContentRegionAvail().y;

			if (this->frame_width * this->frame_height > 0) {
				if (!this->paused) {
					this->frame = this->render();
				}
				if (frame) {
					ImGui::Image(frame->GetDescriptorSet(), { (float)frame->GetWidth(), (float)frame->GetHeight() }, ImVec2(0, 1), ImVec2(1, 0));
				}
			}
		} ImGui::End();
		ImGui::PopStyleVar();

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
	Camera camera{60.f, 0.1f, 100.f};
	Scene scene;
	uint32_t frame_width = 0, frame_height = 0;
	
	std::shared_ptr<Walnut::Image> frame;

	float ltime = 0.f;
	float lscroll = ImGui::GetIO().MouseWheel;
	bool paused{ false };


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