#include "Graphic/Render.hpp"
#include "Graphic/FrameBuffer.hpp"

#include "GL/glew.h"
#include "GL/wglew.h"

#include "global.hpp"

#include "std/vector.hpp"

xstd::vector<Vector3f> generate_ssao_samples(size_t n) noexcept {
	std::uniform_real_distribution<float> unit(0.f, 1.f);
	std::uniform_real_distribution<float> angle(0.f, 3.1415926);
	std::default_random_engine generator;
	xstd::vector<Vector3f> result;
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

		xstd::vector<Vector3f> noise;

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

void edge_highlight(
	G_Buffer& g_buffer, Texture_Buffer& buffer, render::Render_Param& param
) noexcept;

void lighting(
	G_Buffer& g_buffer,
	Texture_Buffer& edge_buffer,
	HDR_Buffer& out_buffer,
	render::Render_Param& param
) noexcept;

void render_transparent(render::Orders& orders, render::Render_Param& param) noexcept;

void motion_blur(
	G_Buffer& g_buffer,
	HDR_Buffer& hdr_buffer,
	HDR_Buffer& out_buffer,
	render::Render_Param& param
) noexcept;

void gaussian_blur(
	size_t texture, HDR_Buffer& out_buffer, render::Render_Param& param
) noexcept;

void tone_mapping(
	HDR_Buffer& hdr_buffer,
	HDR_Buffer& bloom_buffer,
	Texture_Buffer& out_buffer,
	render::Render_Param& param
) noexcept;

void render::render_orders(render::Orders& orders, render::Render_Param param) noexcept {
	auto buffer_size = Environment.buffer_size;
	
	static Texture_Buffer texture_target(buffer_size, "Texture Target");
	static Texture_Buffer edge_buffer(buffer_size, "Edge Buffer");
	static HDR_Buffer     motion_buffer(buffer_size, 1, "Motion Buffer");
	static G_Buffer       g_buffer(buffer_size);
	static HDR_Buffer     hdr_buffer(buffer_size, 1, "HDR Buffer");
	static HDR_Buffer     transparent_buffer(buffer_size, 2, "Transparent Buffer");
	static HDR_Buffer     bloom_buffer(buffer_size, 1, "Bloom Buffer");

	if (g_buffer.n_samples != param.n_samples) g_buffer = G_Buffer(buffer_size, param.n_samples);
	auto& shader = asset::Store.get_shader(asset::Shader_Id::Simple);

	glViewport(0, 0, (GLsizei)buffer_size.x, (GLsizei)buffer_size.y);

	g_buffer.set_active();
	g_buffer.clear({});
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	render_world(orders, param);

	edge_highlight(g_buffer, edge_buffer, param);
	lighting(g_buffer, edge_buffer, hdr_buffer, param);

	transparent_buffer.set_active();
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	render_transparent(orders, param);
	gaussian_blur(transparent_buffer.color2_buffer, bloom_buffer, param);

	motion_blur(g_buffer, hdr_buffer, motion_buffer, param);

	glEnable(GL_BLEND);
	motion_buffer.set_active();
	shader.use();

	transparent_buffer.set_active_texture();
	transparent_buffer.render_quad();

	tone_mapping(motion_buffer, bloom_buffer, texture_target, param);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	texture_target.set_active_texture(0);
	shader.use();
	shader.set_texture(0);

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

	texture_target.render_quad();
	render_ui(orders, param);
}

void render_world(render::Orders& orders, render::Render_Param param) noexcept {
	glEnable(GL_DEPTH_TEST);
	thread_local xstd::vector<render::Order> cam_stack;
	thread_local xstd::vector<render::Batch> batch_stack;
	thread_local xstd::vector<xstd::vector<render::World_Sprite>> world_sprite_batch;
	thread_local xstd::vector<xstd::vector<render::Rectangle>>    rectangle_batch;
	thread_local xstd::vector<xstd::vector<render::Circle>>       circle_batch;
	thread_local xstd::vector<xstd::vector<render::Arrow>>        arrow_batch;
	thread_local xstd::vector<xstd::vector<render::Model>>        model_batch;

	cam_stack.clear();
	batch_stack.clear();
	for (auto& x : world_sprite_batch) x.clear();
	for (auto& x : rectangle_batch) x.clear();
	for (auto& x : circle_batch) x.clear();
	for (auto& x : arrow_batch) x.clear();
	for (auto& x : model_batch) x.clear();

	for (size_t i = 0; i < orders.size(); ++i) {
		auto& x = orders[i];

		switch (x.kind) {
			case render::Order::Color_Mask_Kind: {
				glColorMask(
					x.Color_Mask_.mask.r,
					x.Color_Mask_.mask.g,
					x.Color_Mask_.mask.b,
					x.Color_Mask_.mask.a
				);
				break;
			}
			case render::Order::Depth_Test_Kind: {
				if (x.Depth_Test_.active) glEnable(GL_DEPTH_TEST);
				else                      glDisable(GL_DEPTH_TEST);

				switch (x.Depth_Test_.func) {
					case render::Depth_Test::Equal:   glDepthFunc(GL_EQUAL); break;
					case render::Depth_Test::Less:    glDepthFunc(GL_LESS); break;
					case render::Depth_Test::Greater: glDepthFunc(GL_GREATER); break;
					default: break;
				}
				break;
			};
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
			case render::Order::Particle_Kind:
				render::immediate(x.Particle_, cam_stack.back().Camera3D_, true);
				break;
			case render::Order::Model_Kind: {
				assert(!batch_stack.empty());
				model_batch[batch_stack.size() - 1].push_back(x.Model_);
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
			case render::Order::World_Sprite_Kind: {
				assert(!batch_stack.empty());
				world_sprite_batch[batch_stack.size() - 1].push_back(x.World_Sprite_);
				break;
			}
			case render::Order::Batch_Kind: {
				batch_stack.push_back(x.Batch_);
				if (batch_stack.size() > model_batch.size())
					model_batch.resize(batch_stack.size());
				if (batch_stack.size() > world_sprite_batch.size())
					world_sprite_batch.resize(batch_stack.size());
				break;
			}
			case render::Order::Pop_Batch_Kind: {
				assert(!batch_stack.empty());
				
				if (model_batch[batch_stack.size() - 1].size() > 0) {
					render::immediate(
						model_batch[batch_stack.size() - 1],
						batch_stack.back(),
						cam_stack.back().Camera3D_
					);
				}
				if (world_sprite_batch[batch_stack.size() - 1].size() > 0) {
					auto span =
						xstd::span<render::World_Sprite>(world_sprite_batch[batch_stack.size() - 1]);

					render::immediate(
						span,
						batch_stack.back(),
						cam_stack.back().Camera3D_
					);
				}
				model_batch[batch_stack.size() - 1].clear();
				world_sprite_batch[batch_stack.size() - 1].clear();
				batch_stack.pop_back();
				break;
			}
			default: break;
		}
	}
	glDisable(GL_DEPTH_TEST);
}
void render_transparent(render::Orders& orders, render::Render_Param& param) noexcept {
	glDisable(GL_DEPTH_TEST);
	thread_local xstd::vector<render::Order> cam_stack;

	cam_stack.clear();

	for (size_t i = 0; i < orders.size(); ++i) {
		auto& x = orders[i];

		switch (x.kind) {
			case render::Order::Color_Mask_Kind: {
				glColorMask(
					x.Color_Mask_.mask.r,
					x.Color_Mask_.mask.g,
					x.Color_Mask_.mask.b,
					x.Color_Mask_.mask.a
				);
				break;
			}
			case render::Order::Camera3D_Kind: {
				cam_stack.push_back(x.Camera3D_);
				render::current_camera = cam_stack.back();

				break;
			}
			case render::Order::Pop_Camera3D_Kind: {
				cam_stack.pop_back();
				if (!cam_stack.empty()) render::current_camera = cam_stack.back();
				break;
			}
			case render::Order::Ring_Kind: {
				render::immediate(x.Ring_, cam_stack.back().Camera3D_);
				break;
			}
			case render::Order::Particle_Kind:
				render::immediate(x.Particle_, cam_stack.back().Camera3D_, false);
				break;
			default: break;
		}
	}
	glDisable(GL_DEPTH_TEST);
}

void render_ui(render::Orders& orders, render::Render_Param param) noexcept {
	glDisable(GL_DEPTH_TEST);
	thread_local xstd::vector<render::Order> cam_stack;


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

void edge_highlight(
	G_Buffer& g_buffer, Texture_Buffer& buffer, render::Render_Param& param
) noexcept {
	auto buffer_size = Environment.buffer_size;
	thread_local Texture_Buffer temp_buffer(buffer_size, "Temporary edge highlight");

	temp_buffer.set_active();
	g_buffer.set_active_texture();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

#if 1
	auto& edge_shader = asset::Store.get_shader(asset::Shader_Id::Edge_Glow);
	edge_shader.use();
	edge_shader.set_uniform("buffer_normal", 1);
	edge_shader.set_uniform("buffer_position", 2);
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

	temp_buffer.render_quad();

	buffer.set_active();
	temp_buffer.set_active_texture(0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	auto& blur_shader = asset::Store.get_shader(asset::Shader_Id::Blur);
	blur_shader.use();
	blur_shader.set_uniform("radius", (int)param.edge_blur);
	blur_shader.set_uniform("image", 0);

	buffer.render_quad();
}


void gaussian_blur(
	size_t texture, HDR_Buffer& out_buffer, render::Render_Param& param
) noexcept {
	auto buffer_size = Environment.buffer_size;

	thread_local bool init = false;
	thread_local unsigned int pingpongFBO[2] = {};
	thread_local unsigned int pingpongBuffer[2] = {};
	if (!init) {
		glGenFramebuffers(2, pingpongFBO);
		glGenTextures(2, pingpongBuffer);
		for (unsigned int i = 0; i < 2; i++) {
			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
			glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RGBA16F,
				buffer_size.x,
				buffer_size.y,
				0,
				GL_RGBA,
				GL_FLOAT,
				NULL
			);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0
			);
		}

		init = true;
	}

	for (size_t i = 0; i < 2; ++i) {
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	auto& shaderBlur = asset::Store.get_shader(asset::Shader_Id::Bloom);
	bool horizontal = true, first_iteration = true;
	int amount = 10;
	shaderBlur.use();
	for (unsigned int i = 0; i < amount; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]); 
		shaderBlur.set_uniform("horizontal", horizontal);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(
			GL_TEXTURE_2D, first_iteration ? texture : pingpongBuffer[!horizontal]
		);
		out_buffer.render_quad();

		horizontal = !horizontal;
		first_iteration = false;
	}

	out_buffer.set_active();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pingpongBuffer[1]);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	auto& simple_shader = asset::Store.get_shader(asset::Shader_Id::Simple);
	simple_shader.use();
	simple_shader.set_texture(0);

	out_buffer.render_quad();
}

void lighting(
	G_Buffer& g_buffer,
	Texture_Buffer& edge_buffer,
	HDR_Buffer& out_buffer,
	render::Render_Param& param
) noexcept {
	out_buffer.set_active();
	g_buffer.set_active_texture();
	edge_buffer.set_active_texture(10);
	glClearColor(0.0f, 0.0f, 0.0f, 1.f);
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
}

void motion_blur(
	G_Buffer& g_buffer,
	HDR_Buffer& hdr_buffer,
	HDR_Buffer& out_buffer,
	render::Render_Param& param
) noexcept {
	out_buffer.set_active();
	hdr_buffer.set_active_texture(5);
	g_buffer.set_active_texture();

	auto& motion_shader = asset::Store.get_shader(asset::Shader_Id::Motion);

	float motion_blur = param.motion_scale * param.target_fps / param.current_fps;
	motion_blur = std::clamp(motion_blur, 0.f, 1.f);

	motion_shader.use();
	motion_shader.set_uniform("scale", motion_blur);
	motion_shader.set_uniform("texture_input", 5);
	motion_shader.set_uniform("texture_velocity", 3);

	hdr_buffer.render_quad();
}

void tone_mapping(
	HDR_Buffer& hdr_buffer,
	HDR_Buffer& bloom_buffer,
	Texture_Buffer& texture_target,
	render::Render_Param& param
) noexcept {
	texture_target.set_active();
	hdr_buffer.set_active_texture(0);
	bloom_buffer.set_active_texture(1);

	auto& shader_hdr = asset::Store.get_shader(asset::Shader_Id::HDR);
	shader_hdr.use();
	shader_hdr.set_uniform("gamma", param.gamma);
	shader_hdr.set_uniform("exposure", param.exposure);
	shader_hdr.set_uniform("hdr_texture", 0);
	shader_hdr.set_uniform("add_texture", 1);

	hdr_buffer.render_quad();
}



const char* debug_source_str(GLenum source) {
	static const char* sources[] = {
	  "API",   "Window System", "Shader Compiler", "Third Party", "Application",
	  "Other", "Unknown"
	};
	auto str_idx = std::min(
		(size_t)(source - GL_DEBUG_SOURCE_API),
		(size_t)(sizeof(sources) / sizeof(const char*) - 1)
	);
	return sources[str_idx];
}

const char* debug_type_str(GLenum type) {
	static const char* types[] = {
	  "Error",       "Deprecated Behavior", "Undefined Behavior", "Portability",
	  "Performance", "Other",               "Unknown"
	};

	auto str_idx = std::min(
		(size_t)(type - GL_DEBUG_TYPE_ERROR),
		(size_t)(sizeof(types) / sizeof(const char*) - 1)
	);
	return types[str_idx];
}

const char* debug_severity_str(GLenum severity) {
	static const char* severities[] = {
	  "High", "Medium", "Low", "Unknown"
	};

	auto str_idx = std::min(
		(size_t)(severity - GL_DEBUG_SEVERITY_HIGH),
		(size_t)(sizeof(severities) / sizeof(const char*) - 1)
	);
	return severities[str_idx];
}

void APIENTRY opengl_debug(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei,
	const GLchar* message,
	const void*
) noexcept {
	constexpr GLenum To_Ignore[] = {
		131185,
		131204,
		131140
	};

	constexpr GLenum To_Break_On[] = {
		1280, 1281, 1282, 1286, 131076
	};

	if (std::find(BEG_END(To_Ignore), id) != std::end(To_Ignore)) return;

	printf("OpenGL:\n");
	printf("=============\n");
	printf(" Object ID: ");
	printf("%s\n", std::to_string(id).c_str());
	printf(" Severity:  ");
	printf("%s\n", debug_severity_str(severity));
	printf(" Type:      ");
	printf("%s\n", debug_type_str(type));
	printf("Source:    ");
	printf("%s\n", debug_source_str(source));
	printf(" Message:   ");
	printf("%s\n", message);
	printf("\n");

	if (std::find(BEG_END(To_Break_On), id) != std::end(To_Break_On)) {
		DebugBreak();
	}
}