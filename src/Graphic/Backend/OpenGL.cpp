#include "Graphic/Render.hpp"
#include "Graphic/FrameBuffer.hpp"

#include "GL/glew.h"
#include "GL/wglew.h"

#include "global.hpp"

std::vector<Vector3f> generate_ssao_samples(size_t n) noexcept {
	std::uniform_real_distribution<float> unit(0.f, 1.f);
	std::uniform_real_distribution<float> angle(0.f, 3.1415926);
	std::default_random_engine generator;
	std::vector<Vector3f> result;
	for (unsigned int i = 0; i < n; ++i) {
		float angles[2] = { angle(generator), angle(generator) };
		auto sample = Vector3f::createUnitVector(angles);
		sample *= unit(generator);
		float scale = i / (float)n;
		scale = xstd::lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;

		result.push_back(sample);
	}
	return result;
}

uint32_t get_noise_texture() noexcept {
	static std::optional<uint32_t> noise_texture;
	if (!noise_texture) {

		std::uniform_real_distribution<float> float_dist(-1, 1);
		std::default_random_engine generator(0);

		std::vector<Vector3f> noise;

		for (unsigned int i = 0; i < 16; i++) {
			Vector3f noise_vec;
			noise_vec.x = float_dist(generator);
			noise_vec.z = float_dist(generator);
			noise_vec.z = 1;

			noise.push_back(noise_vec);
		}
		
		noise_texture = 0;
		glGenTextures(1, &*noise_texture);
		glBindTexture(GL_TEXTURE_2D, *noise_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, &noise[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	}
	return *noise_texture;
}

void render_world(render::Orders& orders, render::Render_Param param) noexcept;
void render_ui(render::Orders& orders, render::Render_Param param) noexcept;

void render::render_orders(render::Orders& orders, render::Render_Param param) noexcept {
	auto buffer_size = Environment.buffer_size;
	
	static Texture_Buffer texture_target(buffer_size);
	static Texture_Buffer ssao_buffer(buffer_size, "Edge Buffer");
	static Texture_Buffer blur_buffer(buffer_size, "Blur Buffer");
	static Texture_Buffer motion_buffer(buffer_size, "Motion Buffer");
	static G_Buffer       g_buffer(buffer_size);
	static HDR_Buffer     hdr_buffer(buffer_size);

	if (g_buffer.n_samples != param.n_samples) g_buffer = G_Buffer(buffer_size, param.n_samples);

	glViewport(0, 0, (GLsizei)buffer_size.x, (GLsizei)buffer_size.y);

	g_buffer.set_active();
	g_buffer.clear({});
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	render_world(orders, param);

	ssao_buffer.set_active();
	g_buffer.set_active_texture();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

#if 1
	auto& edge_shader = asset::Store.get_shader(asset::Shader_Id::Edge_Glow);
	edge_shader.use();
	edge_shader.set_uniform("buffer_normal", 1);
	edge_shader.set_uniform("buffer_position", 2);
	edge_shader.set_uniform("buffer_tag", 3);
	edge_shader.set_uniform("cam_pos", param.cam_pos);
#else

	auto samples = generate_ssao_samples(64);
	auto noise_texture = get_noise_texture();
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, noise_texture);

	auto& ssao_shader = asset::Store.get_shader(asset::Shader_Id::SSAO);
	ssao_shader.use();
	ssao_shader.set_uniform("gNormal", 1);
	ssao_shader.set_uniform("gPosition", 2);
	ssao_shader.set_uniform("texNoise", 6);
	ssao_shader.set_uniform("kernelSize", 64);
	ssao_shader.set_uniform("radius", param.ssao_radius);
	ssao_shader.set_uniform("bias", param.ssao_bias);
	ssao_shader.set_uniform("noiseScale", (Vector2f)Environment.window_size / 4);
	for (size_t i = 0; i < 64; ++i)
		ssao_shader.set_uniform("samples[" + std::to_string(i) + "]", samples[i]);
	ssao_shader.set_uniform("projection", game.camera3d.get_projection());
	ssao_shader.set_uniform("view", game.camera3d.get_view());
#endif

	ssao_buffer.render_quad();

	blur_buffer.set_active();
	ssao_buffer.set_active_texture(0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	auto& blur_shader = asset::Store.get_shader(asset::Shader_Id::Blur);
	blur_shader.use();
	blur_shader.set_uniform("radius", (int)param.edge_blur);
	blur_shader.set_uniform("image", 0);

	blur_buffer.render_quad();

	hdr_buffer.set_active();
	g_buffer.set_active_texture();
	blur_buffer.set_active_texture(10);
	glClearColor(0.4f, 0.2f, 0.3f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	auto& shader = asset::Store.get_shader(asset::Shader_Id::Light);
	shader.use();
	shader.set_uniform("ambient_light", Vector4d{1, 1, 1, 1});
	shader.set_uniform("ambient_intensity", 1.f);
	shader.set_uniform("debug", 0);
	shader.set_uniform("n_light_points", 0);
	shader.set_uniform("n_light_dirs", 1);
	shader.set_uniform("buffer_albedo", 0);
	shader.set_uniform("buffer_normal", 1);
	shader.set_uniform("buffer_position", 2);
	shader.set_uniform("buffer_velocity", 3);
	shader.set_uniform("buffer_tag", 4);
	shader.set_uniform("buffer_ssao", 10);

	shader.set_uniform("light_dirs[0].intensity", 1.f);
	shader.set_uniform("light_dirs[0].dir", Vector3f(0, 1, -1).normalize());
	shader.set_uniform("light_dirs[0].color", Vector3f(0.8f, 0.8f, 1.f));

	g_buffer.render_quad();


	// motion blur
	motion_buffer.set_active();
	hdr_buffer.set_active_texture(5);
	g_buffer.set_active_texture();

	auto& motion_shader = asset::Store.get_shader(asset::Shader_Id::Motion);
	motion_shader.use();
	motion_shader.set_uniform("scale", param.motion_scale);
	motion_shader.set_uniform("texture_input", 5);
	motion_shader.set_uniform("texture_velocity", 3);

	hdr_buffer.render_quad();


	texture_target.set_active();
	motion_buffer.set_active_texture(0);

	auto& shader_hdr = asset::Store.get_shader(asset::Shader_Id::HDR);
	shader_hdr.use();
	shader_hdr.set_uniform("gamma", param.gamma);
	shader_hdr.set_uniform("exposure", param.exposure);
	shader_hdr.set_uniform("hdr_texture", 0);
	shader_hdr.set_uniform("transform_color", true);

	Rectanglef viewport_rect{
		0.f, 0.f, (float)Environment.window_size.x, (float)Environment.window_size.y
	};
	viewport_rect = viewport_rect.fitDownRatio({16, 9});

	glViewport(
		(Environment.window_size.x - (GLsizei)viewport_rect.w) / 2,
		(Environment.window_size.y - (GLsizei)viewport_rect.h) / 2,
		(GLsizei)viewport_rect.w,
		(GLsizei)viewport_rect.h
	);

	hdr_buffer.render_quad();


	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	
	shader_hdr.set_uniform("transform_color", false);
	
	texture_target.render_quad();
	hdr_buffer.set_disable_texture();
	render_ui(orders, param);
}

void render_world(render::Orders& orders, render::Render_Param param) noexcept {
	glEnable(GL_DEPTH_TEST);
	thread_local std::vector<render::Order> cam_stack;
	thread_local std::vector<std::vector<render::Rectangle>> rectangle_batch;
	thread_local std::vector<std::vector<render::Circle>>    circle_batch;
	thread_local std::vector<std::vector<render::Arrow>>     arrow_batch;
	thread_local std::vector<std::vector<render::Model>>     model_batch;

	size_t model_batch_stack = 0;

	cam_stack.clear();
	for (auto& x : rectangle_batch) x.clear();
	for (auto& x : circle_batch) x.clear();
	for (auto& x : arrow_batch) x.clear();
	for (auto& x : model_batch) x.clear();

	for (size_t i = 0; i < orders.size(); ++i) {
		auto& x = orders[i];

		switch (x.kind) {
			case render::Order::Pop_Ui_Kind: assert(false); break;
			case render::Order::Push_Ui_Kind: {
				size_t depth = 0;
				for (i++; i < orders.size(); ++i) {
					if (orders[i].kind == render::Order::Push_Ui_Kind) depth--;
					if (orders[i].kind == render::Order::Pop_Ui_Kind) {
						if (depth == 0) break;
						depth--;
					}
				}
				break;
			}
			case render::Order::Camera_Kind: {
				cam_stack.push_back(x.Camera_);

				if (rectangle_batch.size() < cam_stack.size())
					rectangle_batch.resize(cam_stack.size());
				if (circle_batch.size() < cam_stack.size()) circle_batch.resize(cam_stack.size());
				if (arrow_batch.size() < cam_stack.size())  arrow_batch.resize(cam_stack.size());

				render::current_camera = cam_stack.back();
				break;
			}
			case render::Order::Pop_Camera_Kind: {
				if (auto& batch = rectangle_batch[cam_stack.size() - 1]; !batch.empty()) {
					render::immediate(batch);
					batch.clear();
				}
				if (auto& batch = circle_batch[cam_stack.size() - 1]; !batch.empty()) {
					render::immediate(batch);
					batch.clear();
				}
				if (auto& batch = arrow_batch[cam_stack.size() - 1]; !batch.empty()) {
					render::immediate(batch);
					batch.clear();
				}

				cam_stack.pop_back();
				if (!cam_stack.empty()) render::current_camera = cam_stack.back();


				break;
			}
			case render::Order::Camera3D_Kind: {
				cam_stack.push_back(x.Camera3D_);
				render::current_camera = cam_stack.back();
				if (rectangle_batch.size() < cam_stack.size())
					rectangle_batch.resize(cam_stack.size());
				if (circle_batch.size() < cam_stack.size())
					circle_batch.resize(cam_stack.size());
				if (arrow_batch.size() < cam_stack.size())
					arrow_batch.resize(cam_stack.size());

				break;
			}
			case render::Order::Pop_Camera3D_Kind: {
				if (auto& batch = rectangle_batch[cam_stack.size() - 1]; !batch.empty()) {
					render::immediate(batch);
					batch.clear();
				}
				if (auto& batch = circle_batch[cam_stack.size() - 1]; !batch.empty()) {
					render::immediate(batch);
					batch.clear();
				}
				if (auto& batch = arrow_batch[cam_stack.size() - 1]; !batch.empty()) {
					render::immediate(batch);
					batch.clear();
				}

				cam_stack.pop_back();
				if (!cam_stack.empty()) render::current_camera = cam_stack.back();
				break;
			}
			case render::Order::Text_Kind:       render::immediate(x.Text_);      break;
			case render::Order::Sprite_Kind:     render::immediate(x.Sprite_);    break;
			case render::Order::Model_Kind: {
				assert(model_batch_stack > 0);
				model_batch[model_batch_stack - 1].push_back(x.Model_);
				break;
			}
			case render::Order::Arrow_Kind: {
				arrow_batch[cam_stack.size() - 1].push_back(x.Arrow_);
				break;
			}
			case render::Order::Circle_Kind: {
				circle_batch[cam_stack.size() - 1].push_back(x.Circle_);
				break;
			}
			case render::Order::Rectangle_Kind: {
				rectangle_batch[cam_stack.size() - 1].push_back(x.Rectangle_);
				break;
			}
			case render::Order::Clear_Depth_Kind: {
				glClear(GL_DEPTH_BUFFER_BIT);
				break;
			}
			case render::Order::Push_Batch_Kind: {
				model_batch_stack++;
				if (model_batch_stack >= model_batch.size()) model_batch.resize(model_batch_stack);
				break;
			}
			case render::Order::Pop_Batch_Kind: {
				assert(model_batch_stack > 0);
				
				render::immediate(model_batch[model_batch_stack - 1]);
				model_batch[model_batch_stack - 1].clear();
				model_batch_stack--;
				break;
			}
			default: break;
		}
	}
	glDisable(GL_DEPTH_TEST);
}

void render_ui(render::Orders& orders, render::Render_Param param) noexcept {
	glDisable(GL_DEPTH_TEST);
	thread_local std::vector<render::Order> cam_stack;


	cam_stack.clear();
	for (size_t i = 0; i < orders.size(); ++i) {
		auto& x = orders[i];
		if (x.kind != render::Order::Push_Ui_Kind) continue;

		size_t depth = 0;
		for (
			;
			i < orders.size() && !(orders[i].kind == render::Order::Pop_Ui_Kind && depth-- == 0);
			++i
		) {
			auto& y = orders[i];


			switch (y.kind) {
				case render::Order::Rectangle_Kind: {
					render::immediate(y.Rectangle_);
					break;
				}
				case render::Order::Sprite_Kind: {
					render::immediate(y.Sprite_);
					break;
				}
				case render::Order::Text_Kind: {
					render::immediate(y.Text_);
					break;
				}
				case render::Order::Camera_Kind: {
					cam_stack.push_back(y.Camera_);
					render::current_camera = cam_stack.back();
					break;
				}
				case render::Order::Pop_Camera_Kind: {
					cam_stack.pop_back();
					if (!cam_stack.empty()) render::current_camera = cam_stack.back();
					break;
				}
				default: break;
			}
		}
	}
}