#include <chrono>
#include <vector>
#include <thread>
#include <algorithm>

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

	enum {
		Event_None = 0,
		Event_CameraMoved = 1 << 0,
		Event_WindowResize = 1 << 1,
		Event_ParamChange = 1 << 2
	};

	virtual void OnUpdate(float ts) override {
		if (this->camera.OnUpdate(ts)) {
			this->events |= Event_CameraMoved;
		}
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

			if (this->frame_width != this->rstack.width || this->frame_height != this->rstack.height) {
				this->events |= Event_WindowResize;
			}
			if (!this->pause) {
				if (needs_reset) {
					this->renderer.cancelRender();
					this->rstack.resize_lock.lock_shared();
					memset(this->rstack.accum_ratio, 0, this->rstack.width * this->rstack.height * sizeof(glm::vec4));
					this->rstack.resize_lock.unlock_shared();
				}
				if (this->frame && !(this->renderer.properties.render_flags & Renderer::RenderMode_Sync_Frame)) {
					this->rstack.resize_lock.lock_shared();	// try lock
					this->frame->SetData(this->rstack.buffer);
					this->rstack.resize_lock.unlock_shared();
				}	// idk what to do for syncing yet...
			}
			if (this->frame) {	// always render frame if valid
				this->rstack.resize_lock.lock_shared();
				ImGui::Image(
					frame->GetDescriptorSet(),
					{ (float)frame->GetWidth(), (float)frame->GetHeight() },
					ImVec2(0, 1), ImVec2(1, 0)
				);
				this->rstack.resize_lock.unlock_shared();
			}
		} ImGui::End();
		ImGui::PopStyleVar();

		if (this->renderer.properties.antialias_samples > this->rstack.depth) {
			this->events |= Event_ParamChange;
		}
		if (this->events && !this->update_running) {
			std::thread(&RenderLayer::updateBuffers, this, this->events).detach();
			this->events = Event_None;
		}

	}
	virtual void OnAttach() override {
		this->render_thread = std::thread(&RenderLayer::renderLoop, this);
	}
	virtual void OnDetach() override {
		this->exit = true;
		this->render_thread.join();
		if (this->rstack.width || this->rstack.height || this->rstack.depth) {
			delete[] this->rstack.directions;
			delete[] this->rstack.accum_ratio;
			delete[] this->rstack.buffer;
		}
	}


protected:
	void renderLoop() {
		while (this->frame_width == 0 || this->frame_height == 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));	// wait until window has been initialized
		}
		while (!this->exit) {
			if (this->pause) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			} else {
				this->renderer.render(this->scene, this->rstack);
			}
		}
		this->exit = false;
	}
	void updateBuffers(uint32_t events) {
		this->update_running = true;
		uint32_t
			w = this->frame_width,
			h = this->frame_height,
			d = std::max((int)this->renderer.properties.antialias_samples, (int)this->rstack.depth);

		if (events & Event_CameraMoved) {
			this->viewport.UpdateView(this->camera);
		}
		if (events & (Event_CameraMoved | Event_WindowResize)) {
			this->viewport.UpdateProjection(this->camera, w, h);
		}
		// if Event_CameraMoved | Event_WindowResize | Event_ParmChange --> currently all events
		glm::vec3* temp = new glm::vec3[w * h * d + 1];
		temp[0] = this->camera.GetPosition();
		this->viewport.GenerateDirections(temp + 1, d, w, h);

		if (events & Event_WindowResize) {
			this->renderer.cancelRender();
			this->rstack.move_lock.lock();
			this->rstack.resize_lock.lock();
			delete[] this->rstack.directions;
			delete[] this->rstack.accum_ratio;
			delete[] this->rstack.buffer;
			this->rstack.directions = temp;
			this->rstack.accum_ratio = new glm::vec4[w * h];
			this->rstack.buffer = new uint32_t[w * h];
			if (this->frame) {
				this->frame->Resize(w, h);
			} else {
				this->frame = new Walnut::Image(w, h, Walnut::ImageFormat::RGBA);
			}
			this->rstack.width = w;
			this->rstack.height = h;
			this->rstack.depth = d;
			this->rstack.move_lock.unlock();
			this->rstack.resize_lock.unlock();
		} else {
			this->renderer.pauseRender();
			this->rstack.move_lock.lock();
			delete[] this->rstack.directions;
			this->rstack.directions = temp;
			if (events & Event_CameraMoved) {
				memset(this->rstack.accum_ratio, 0, w * h * sizeof(glm::vec4));	// reset accumulation because the camera has moved
			}
			this->rstack.move_lock.unlock();
			this->renderer.resumeRender();
		}

		this->update_running = false;
	}

private:
	Renderer renderer;
	Camera camera{60.f, 0.1f, 100.f};
	Scene scene;
	MaterialManager mats;
	TextureManager texts;

	RenderStack rstack;
	CameraView viewport;

	Walnut::Image* frame{ nullptr };
	uint32_t frame_width{ 0 }, frame_height{ 0 };

	std::thread render_thread;
	std::atomic_bool pause{ false }, exit{ false }, update_running{ false };

	uint32_t events{ Event_None };
	float ltime{ 0.f };


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