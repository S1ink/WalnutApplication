#include <chrono>
#include <vector>
#include <thread>

#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Image.h"
#include "Walnut/Timer.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Render.h"
#include "Camera.h"
#include "Scene.h"
#include "Objects.h"


class RenderLayer : public Walnut::Layer
{
public:
	RenderLayer() : scene(demo) {}


	virtual void OnUpdate(float ts) override {
		static bool cam_updated;
		cam_updated |= this->camera.OnUpdate(ts);
	}
	virtual void OnUIRender() override {

		using hrc = std::chrono::high_resolution_clock;
		static hrc::time_point ref = hrc::now();
		bool needs_reset = false;
		
	// Render settings window
		ImGui::Begin("Render Options"); {
			ImGui::Text("FPS: %.3f", 1e9 / (hrc::now() - ref).count());
			ref = hrc::now();
			if (ImGui::Button(this->pause ? "Unpause Render" : "Pause Render")) { this->pause = !this->pause; }
			ImGui::SameLine();
			if (ImGui::Button("Restart Render")) { needs_reset = true; }
			ImGui::Separator();
			needs_reset |= this->renderer.invokeGuiOptions();
		} ImGui::End();
	// Call Scene and property editor windows
		ImGui::Begin("Scene"); {
			needs_reset |= this->scene.invokeGuiOptions();
		} ImGui::End();
		ImGui::Begin("Materials"); {
			this->mats.invokeGui();
		} ImGui::End();
		ImGui::Begin("Textures"); {
			this->texts.invokeGui();
		} ImGui::End();
	// Display the render
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Render"); {
			this->frame_width = ImGui::GetContentRegionAvail().x;
			this->frame_height = ImGui::GetContentRegionAvail().y;

			if (this->frame_width * this->frame_height > 0) {
				if (!this->pause) {
					if (needs_reset || (frame && (this->frame_width != this->frame->GetWidth() || this->frame_height != this->frame->GetHeight()))) {	// if the scene changed or the window size changed
						this->renderer.resetRender();
					}
					this->frame = this->renderer.getOutput();
				}
				if (frame) {
					ImGui::Image(frame->GetDescriptorSet(), { (float)frame->GetWidth(), (float)frame->GetHeight() }, ImVec2(0, 1), ImVec2(1, 0));
				}
			}
		} ImGui::End();
		ImGui::PopStyleVar();

	}
	virtual void OnAttach() override {
		this->render_thread = std::thread(&RenderLayer::renderLoop, this);
	}
	virtual void OnDetach() override {
		this->exit = true;
		this->render_thread.join();
	}


protected:
	void renderLoop() {
		while (this->frame_width == 0 || this->frame_height == 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));	// wait until window has been initialized
		}
		this->camera.OnResize(this->frame_width, this->frame_height, this->renderer.properties.antialias_samples);
		this->renderer.resize(this->frame_width, this->frame_height);
		while (!this->exit) {
			if (this->pause) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			} else {
				this->camera.OnResize(this->frame_width, this->frame_height);
				this->renderer.resize(this->frame_width, this->frame_height);
				this->renderer.render(this->scene, this->camera);
			}
		}
		this->exit = false;
	}

private:
	Renderer renderer;
	Camera camera{60.f, 0.1f, 100.f};
	Scene scene;
	MaterialManager mats;
	TextureManager texts;

	std::shared_ptr<Walnut::Image> frame;
	uint32_t frame_width = 0, frame_height = 0;

	std::thread render_thread;
	std::atomic_bool pause{ false }, exit{ false };

	float ltime = 0.f;


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
		if (demo)
		{
			ImGui::ShowDemoWindow(&demo);
		}
	});
	return app;
}