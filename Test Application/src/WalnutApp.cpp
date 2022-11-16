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
	RenderLayer() : scene(demo) {}


	virtual void OnUpdate(float ts) override {
		if (this->camera.OnUpdate(ts)) {
			this->renderer.resetAccumulatedFrames();
			//this->renderer.updateRandomRays(this->camera);
		}
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
			ImGui::Checkbox("Accumulate Samples", &this->renderer.accumulate);
			ImGui::SameLine();
			if (ImGui::Button("Reset Accumulation")) {
				this->renderer.resetAccumulatedFrames();
			}
			ImGui::Separator();
			ImGui::ColorEdit3("Sky Color", glm::value_ptr(this->scene.sky_color));
			if (ImGui::DragInt("Bounce Limit", &Renderer::MAX_BOUNCES, 1.f, 1, 100) && Renderer::MAX_BOUNCES < 1) { Renderer::MAX_BOUNCES = 1; }
			if (ImGui::DragInt("Samples", &Renderer::SAMPLE_RAYS, 1.f, 1, 100) && Renderer::SAMPLE_RAYS < 1) { Renderer::SAMPLE_RAYS = 1; }
			ImGui::Separator();
			this->scene.invokeGuiOptions();
			

		} ImGui::End();
		ImGui::Begin("Materials"); {
			ImGui::PushID("default_mat");
			if (ImGui::CollapsingHeader("Default Material")) {
				PhysicalBase::DEFAULT->invokeGuiOptions();
			}
			ImGui::PopID();
			ImGui::PushID("light_mat");
			if (ImGui::CollapsingHeader("Light Source Material")) {
				PhysicalBase::LIGHT->invokeGuiOptions();
			}
			ImGui::PopID();
			this->mats.invokeGui();
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
			if (this->renderer.resize(this->frame_width, this->frame_height)) {
				this->camera.OnResize(this->frame_width, this->frame_height);
				//this->renderer.updateRandomRays(this->camera);
			}
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
	MaterialManager mats;
	uint32_t frame_width = 0, frame_height = 0;
	std::thread rendt;
	
	std::shared_ptr<Walnut::Image> frame;

	float ltime = 0.f;
	bool paused{ false }, desync_render{ false }, unshaded{ false };


};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Application";
	spec.Width = 1280;
	spec.Height = 720;

	Walnut::Application* app = new Walnut::Application(spec);
	static bool demo{ false };
	app->PushLayer<RenderLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::MenuItem("Demo", NULL, &demo);
			ImGui::EndMenu();
		}
		if (demo) {
			ImGui::ShowDemoWindow(&demo);
		}
	});
	return app;
}