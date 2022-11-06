#include <chrono>
#include <vector>
#include <thread>

#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Image.h"
#include "Walnut/Timer.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Renderer.h"
#include "Camera.h"
#include "Scene.h"
#include "Objects.h"


class RenderLayer : public Walnut::Layer
{
public:
	RenderLayer() : scene{
		{
			objects
		},
		{
			std::vector<int>((int)this->scene.objects.size(), -1)
		},
		{
			{1}
		},
		{
			new Sphere(glm::vec3{-1, -1, -1}),
			new Sphere(glm::vec3{-4.f, 3.f, 2.f}, 1.f, glm::vec3{0.f, 1.f, 0.5f})/*,
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
			ImGui::SameLine();
			if (ImGui::Button(this->desync_render ? "Sync Rendering" : "Desync Rendering")) {
				if (this->desync_render = !this->desync_render) {
					this->rendt = std::move(std::thread([this]() {
						while (this->desync_render) {
							if (this->paused) {
								std::this_thread::sleep_for(std::chrono::milliseconds(100));
							} else {
								this->renderer.resize(this->frame_width, this->frame_height);
								this->camera.OnResize(this->frame_width, this->frame_height);
								if (this->unshaded) {
									this->renderer.renderUnshaded(this->scene, this->camera);
								}
								else {
									this->renderer.render(this->scene, this->camera);
								}
							}
						}
					}));
				} else {
					this->rendt.join();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button(this->unshaded ? "Enable RT" : "Disable RT")) {
				this->unshaded = !this->unshaded;
			}
			ImGui::Separator();
			ImGui::ColorEdit3("Sky Color", glm::value_ptr(Renderer::SKY_COLOR));
			if (ImGui::DragInt("Bounce Limit", &Renderer::MAX_BOUNCES, 1.f, 1, 100) && Renderer::MAX_BOUNCES < 1) { Renderer::MAX_BOUNCES = 1; }
			if (ImGui::DragInt("Samples", &Renderer::SAMPLE_RAYS, 1.f, 1, 100) && Renderer::SAMPLE_RAYS < 1) { Renderer::SAMPLE_RAYS = 1; }
			ImGui::Separator();
			for (size_t i = 0; i < this->scene.objects.size(); i++) {
				ImGui::PushID(i);
				if (ImGui::CollapsingHeader(("Obj " + std::to_string(i)).c_str())) {
					this->scene.objects[i]->invokeOptions();
					if (ImGui::DragInt("Material ID", &this->scene.obj_mats[i], 1.f, -1, (int)this->scene.materials.size() - 1) && this->scene.obj_mats[i] < this->scene.materials.size()) {
						this->scene.objects[i]->mat = (this->scene.obj_mats[i] < 0 ? &_DEFAULT_MAT : &this->scene.materials[this->scene.obj_mats[i]]);
					}
				}
				ImGui::PopID();
			}
			ImGui::Separator;
			for (size_t i = 0; i < this->scene.materials.size(); i++) {
				ImGui::PushID(i);
				if (ImGui::CollapsingHeader(("Mat " + std::to_string(i)).c_str())) {
					ImGui::DragFloat("Roughness", &this->scene.materials[i].roughness, 0.005, 0.f, 1.f);
					ImGui::DragFloat("Matallic", &this->scene.materials[i].metallic, 0.005, 0.f, 1.f);
					ImGui::DragFloat("Luminance", &this->scene.materials[i].luminance, 0.005, 0.f, 1.f);
				}
				ImGui::PopID();
			}
			if (ImGui::Button("Add Material")) { this->scene.materials.emplace_back(); }

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
		if (!this->desync_render) {
			this->renderer.resize(this->frame_width, this->frame_height);
			this->camera.OnResize(this->frame_width, this->frame_height);
			if (this->unshaded) {
				this->renderer.renderUnshaded(this->scene, this->camera);
			} else {
				this->renderer.render(this->scene, this->camera);
			}
		}
		return this->renderer.getOutput();
	}

private:
	Renderer renderer;
	Camera camera{60.f, 0.1f, 100.f};
	Scene scene;
	uint32_t frame_width = 0, frame_height = 0;
	std::thread rendt;
	
	std::shared_ptr<Walnut::Image> frame;

	float ltime = 0.f;
	float lscroll = ImGui::GetIO().MouseWheel;
	bool paused{ false }, desync_render{ false }, unshaded{ false };


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