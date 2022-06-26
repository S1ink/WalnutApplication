#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Image.h"
#include "Walnut/Timer.h"
#include <glm/glm.hpp>

#include "Renderer.h"


class RenderLayer : public Walnut::Layer
{
public:
	virtual void OnUIRender() override {
		
		ImGui::Begin("Options");
		ImGui::Text("FPS: %.3f", 1000 / this->ltime);
		this->renderer.moveOrigin(
			{
				(ImGui::IsKeyDown(ImGuiKey_::ImGuiKey_RightArrow) - ImGui::IsKeyDown(ImGuiKey_::ImGuiKey_LeftArrow)) * 0.1f,
				(ImGui::IsKeyDown(ImGuiKey_::ImGuiKey_UpArrow) - ImGui::IsKeyDown(ImGuiKey_::ImGuiKey_DownArrow)) * 0.1f,
				(ImGui::GetIO().MouseWheel - this->lscroll) / -10.f
			}
		);
		this->renderer.moveBrightness(
			(ImGui::GetIO().MouseDown[1] - ImGui::GetIO().MouseDown[0]) * 10.f
		);
		ImGui::End();

		ImGui::Begin("Render");

		this->frame_width = ImGui::GetContentRegionAvail().x;
		this->frame_height = ImGui::GetContentRegionAvail().y;

		//this->render();
		
		auto frame = this->renderer.getOutput();
		if (frame) {
			ImGui::Image(frame->GetDescriptorSet(), { (float)frame->GetWidth(), (float)frame->GetHeight() }, ImVec2(0, 1), ImVec2(1, 0));
		}

		ImGui::End();

		this->render();

		//ImGui::ShowDemoWindow();
	}
protected:
	void render() {
		Walnut::Timer time;
		this->renderer.resize(this->frame_width, this->frame_height);
		this->renderer.render();
		this->ltime = time.ElapsedMillis();
	}

private:
	Renderer renderer;
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