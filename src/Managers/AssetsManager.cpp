#include "AssetsManager.hpp"

#include <mutex>

#include "xstd.hpp"
#include "global.hpp"

#include "OS/file.hpp"
#include "OS/OpenGL.hpp"

#include "dyn_struct.hpp"

using namespace asset;

namespace asset {
	Store_t Store;
}

[[nodiscard]] Texture* Store_t::get_normal(size_t k) const noexcept {
	return textures.at(k).normal ? textures.at(k).normal.get() : nullptr;
}

[[nodiscard]] Texture& Store_t::get_albedo(size_t k) noexcept {
	return textures.at(k).albedo;
}
[[nodiscard]] Shader& Store_t::get_shader(size_t k) noexcept {
	return shaders.at(k).asset;
}

[[nodiscard]] std::optional<size_t> Store_t::load_texture(std::filesystem::path path) noexcept {
	auto x = xstd::uuid();
	return load_texture(x, path) ? std::optional{ x } : std::nullopt;
}

[[nodiscard]] bool Store_t::load_texture(size_t k, std::filesystem::path path) noexcept {
	auto& new_texture = textures[k];
	new_texture.path = path;
	bool loaded = new_texture.albedo.load_file(path);

	auto normal_path = path;
	normal_path.replace_filename(path.stem().string() + "_n.png");

	if (std::filesystem::is_regular_file(normal_path)) {
		new_texture.normal = std::make_unique<Texture>();
		new_texture.normal->load_file(normal_path);
	}

	if (loaded) {
		textures_loaded[std::filesystem::weakly_canonical(path).string()] = k;
		return true;
	}
	else {
		return false;
	}
}

[[nodiscard]] std::optional<size_t> Store_t::load_shader(
	std::filesystem::path vertex
) noexcept {
	auto x = xstd::uuid();
	return load_shader(x, vertex) ? std::optional{x} : std::nullopt;
}
[[nodiscard]] std::optional<size_t> Store_t::load_shader(
	std::filesystem::path vertex, std::filesystem::path fragment
) noexcept {
	auto x = xstd::uuid();
	return load_shader(x, vertex, fragment) ? std::optional{x} : std::nullopt;
}
[[nodiscard]] std::optional<size_t> Store_t::load_shader(
	std::filesystem::path vertex,
	std::filesystem::path fragment,
	std::filesystem::path geometry
) noexcept {
	auto x = xstd::uuid();
	return load_shader(x, vertex, fragment, geometry) ? std::optional{x} : std::nullopt;
}


[[nodiscard]] bool Store_t::load_shader(
	size_t k, std::filesystem::path vertex
) noexcept {
	auto new_shader = Shader::create_shader(vertex);
	if (!new_shader) return false;

	Asset_Shader s{
		std::move(*new_shader),
		std::filesystem::weakly_canonical(vertex)
	};


	shaders.insert({ k, std::move(s) });

	return true;
}

[[nodiscard]] bool Store_t::load_shader(
	size_t k, std::filesystem::path vertex, std::filesystem::path fragment
) noexcept {
	auto new_shader = Shader::create_shader(vertex, fragment);
	if (!new_shader) return false;

	Asset_Shader s{
		std::move(*new_shader),
		std::filesystem::weakly_canonical(vertex),
		std::filesystem::weakly_canonical(fragment)
	};


	shaders.insert({ k, std::move(s) });

	return true;
}

[[nodiscard]] bool Store_t::load_shader(
	size_t k,
	std::filesystem::path vertex,
	std::filesystem::path fragment,
	std::filesystem::path geometry
) noexcept {
	auto new_shader = Shader::create_shader(vertex, fragment, geometry);
	if (!new_shader) return false;

	Asset_Shader s{
		std::move(*new_shader),
		std::filesystem::weakly_canonical(vertex),
		std::filesystem::weakly_canonical(fragment),
		std::filesystem::weakly_canonical(geometry)
	};

	shaders.insert({ k, std::move(s) });

	return true;
}

[[nodiscard]] size_t Store_t::make_texture() noexcept {
	auto k = xstd::uuid();

	textures.emplace(k, Asset_Texture{});

	return k;
}

void Store_t::monitor_path(std::filesystem::path dir) noexcept {
#ifndef WEB
	static int attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 0,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		//WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
		0
	};

	auto gl = wglCreateContextAttribsARB(
		(HDC)platform::handle_dc_window, (HGLRC)platform::main_opengl_context, attribs
	);

	file::monitor_dir(
		[gl] {
			if (!wglMakeCurrent(
				(HDC)platform::handle_dc_window, gl
			)) std::abort();
		},
		dir,
		[&, d = dir] (std::filesystem::path path) -> bool {
			if (stop) return true;
			Main_Mutex.lock();
			defer { Main_Mutex.unlock(); };
			
			path = std::filesystem::canonical(d / path);

			{
				// >SLOW(Tackwin)
				auto suffixes = xstd::split(path.stem().string(), "_");

				auto size_t = path.parent_path() / (suffixes.front() + path.extension().string());

				auto it = textures_loaded.find(size_t.string());
				
				if (it != END(textures_loaded) && textures.count(it->second)) {
					auto& asset = textures.at(it->second);

					if (suffixes.size() == 1) {
						asset.albedo.load_file(path.string());
						return false;
					}

					auto last = suffixes.back();
					if (last == "n") {
						if (!asset.normal) asset.normal = std::make_unique<Texture>();
						asset.normal->load_file(path.string());
					}
				}

			}

			for (auto& [_, x] : shaders){
				if (x.fragment == path) {
					if (!x.asset.load_fragment(path)) continue;

					if (x.asset.build_shaders()) printf("Rebuilt shader.\n");
					else printf("Faield to rebuild shader.\n");
				}
				else if (x.vertex == path) {

					if (!x.asset.load_vertex(path)) continue;

					if (x.asset.build_shaders()) printf("Rebuilt shader.\n");
					else printf("Faield to rebuild shader.\n");
				}
				else if (x.geometry == path) {

					if (!x.asset.load_geometry(path)) continue;

					if (x.asset.build_shaders()) printf("Rebuilt shader.\n");
					else printf("Failed to rebuild shader.\n");
				}
			}

			for (auto& [_, x] : dlls) {
				if (std::filesystem::canonical(x.path) == path) {
					auto new_dll = ::load_dll(path);
					if (!new_dll) {
						auto str = path.string();
						printf("Failed to hot reload dll %s.\n", str.c_str());
						continue;
					}
					x.asset = std::move(*new_dll);
				}
			}

			for (auto& [_, x] : objects) {
				if (std::filesystem::canonical(x.path) == path) {
					auto new_obj = Object::load_from_file(path);
					if (!new_obj) {
						auto str = path.string();
						printf("Failed to hot reload dll %s.\n", str.c_str());
						continue;
					}
					x.asset = std::move(*new_obj);
				}
			}

			return false;
		}
	);
#endif
}

void Store_t::load_known_textures() noexcept {

	std::optional<size_t> opt;

#define X(str, x)\
	printf("Loading " str " ... ");\
	opt = load_texture(str);\
	if (opt) {\
		Texture_Id::x = *opt;\
		printf("sucess :) !\n");\
	}\
	else {\
		printf("failed :( !\n");\
	}

	X("assets/textures/gold_icon.png",  Gold_Icon);
	X("assets/textures/back_icon.png",  Back_Icon);
	X("assets/textures/null_icon.png",  Null_Icon);
	X("assets/textures/build_icon.png", Build_Icon);
	X("assets/textures/up_icon.png",    Up_Icon);
	X("assets/textures/left_icon.png",  Left_Icon);
	X("assets/textures/down_icon.png",  Down_Icon);
	X("assets/textures/right_icon.png", Right_Icon);
	X("assets/textures/archer_build_icon.png", Archer_Build_Icon);
	X("assets/textures/splash_build_icon.png", Splash_Build_Icon);
	X("assets/textures/cancel_icon.png", Cancel_Icon);
	X("assets/textures/send_icon.png", Send_Icon);
	X("assets/textures/dummy.png", Dummy);
	X("assets/textures/palette.png", Palette);
	X("assets/textures/methane_icon.png", Methane_Icon);
	X("assets/textures/dice_icon.png", Dice_Icon);
	X("assets/textures/first_icon.png", First_Icon);
	X("assets/textures/target_icon.png", Target_Icon);
	X("assets/textures/closest_icon.png", Closest_Icon);
	X("assets/textures/farthest_icon.png", Farthest_Icon);
	X("assets/textures/range1_icon.png", Range1_Icon);
	X("assets/textures/sharp_icon.png", Sharp_Icon);
	X("assets/textures/volter_icon.png", Volter_Icon);
	X("assets/textures/carbon_icon.png", Carbon_Icon);
	X("assets/textures/hydrogen_icon.png", Hydrogen_Icon);

#undef X
}

void Store_t::load_known_fonts() noexcept {
	std::optional<size_t> opt;

#define X(str, x)\
	printf("Loading " #x " ... ");\
	opt = load_font(str);\
	if (opt) {\
		Font_Id::x = *opt;\
		printf("sucess :) !\n");\
	}\
	else {\
		printf("failed :( !\n");\
	}
	X("assets/fonts/consolas_regular_100.json", Consolas);

#undef X
}

void Store_t::load_known_shaders() noexcept {

	std::optional<size_t> opt;

#define X(f, v, g, x)\
	printf("Loading " #x " shader... ");\
	if (*v == '\0') opt = load_shader(f);\
	else if (*g == '\0') opt = load_shader(f, v);\
	else opt = load_shader(f, v, g);\
	if (opt) {\
		Shader_Id::x = *opt;\
		printf("sucess :) !\n");\
	}\
	else {\
		printf("failed :( !\n");\
	}

	X("assets/shaders/default.vertex", "assets/shaders/default.fragment", "", Default);
	X("assets/shaders/default_3d.vertex", "assets/shaders/default_3d.fragment", "", Default_3D);
	X(
		"assets/shaders/default_3d_batched.vertex",
		"assets/shaders/default_3d_batched.fragment",
		"",
		Default_3D_Batched
	);
	X(
		"assets/shaders/default_batched.vertex",
		"assets/shaders/default_batched.fragment",
		"",
		Default_Batched
	);
	X(
		"assets/shaders/primitive_3d_batched.vertex",
		"assets/shaders/primitive_3d_batched.fragment",
		"",
		Primitive_3D_Batched
	);
	X("assets/shaders/simple.vertex", "assets/shaders/edge_glow.fragment", "", Edge_Glow);
	X("assets/shaders/simple.vertex", "assets/shaders/motion.fragment",    "", Motion);
	X("assets/shaders/light.vertex",  "assets/shaders/light.fragment",     "", Light);
	X("assets/shaders/simple.vertex", "assets/shaders/ssao.fragment",      "", SSAO);
	X("assets/shaders/simple.vertex", "assets/shaders/blur.fragment",      "", Blur);
	X("assets/shaders/simple.vertex", "assets/shaders/hdr.fragment",       "", HDR);
	X("assets/shaders/simple.vertex", "assets/shaders/simple.fragment",    "", Simple);
	X("assets/shaders/depth_prepass.vertex", "",    "", Depth_Prepass);
	X("assets/shaders/ring.vertex", "assets/shaders/ring.fragment",    "", Ring);
	X("assets/shaders/simple.vertex", "assets/shaders/bloom.fragment",    "", Bloom);
	X("assets/shaders/simple.vertex", "assets/shaders/additive.fragment",    "", Additive);
	X(
		"assets/shaders/particle.vertex",
		"assets/shaders/particle_deferred.fragment",
		"",
		Particle_Deferred
	);
	X(
		"assets/shaders/particle.vertex",
		"assets/shaders/particle_bloom.fragment",
		"",
		Particle_Bloom
	);
#undef X
}
void Store_t::load_known_objects() noexcept {
	std::optional<size_t> opt;

#define X(str, x)\
	printf("Loading " #x " ... ");\
	opt = load_object(str);\
	if (opt) {\
		Object_Id::x = *opt;\
		printf("sucess :) !\n");\
	}\
	else {\
		printf("failed :( !\n");\
	}
	X("assets/model/methane.ply", Methane);
	X("assets/model/empty_tile.ply", Empty_Tile);
	X("assets/model/range1.ply", Range1);
	X("assets/model/proj.ply", Projectile);
	X("assets/model/sharp.ply", Sharp);
	X("assets/model/water.ply", Water);
	X("assets/model/base.ply", Base);
	X("assets/model/photon.ply", Photon);
	X("assets/model/oxygen.ply", Oxygen);
	X("assets/model/Volter.ply", Volter);
	X("assets/model/Electron.ply", Electron);
	X("assets/model/ethane.ply", Ethane);
	X("assets/model/propane.ply", Propane);
	X("assets/model/butane.ply", Butane);
	X("assets/model/plane.ply", Plane);
#undef X
}

void Store_t::load_known_sounds() noexcept {
	std::optional<size_t> opt;

#define X(str, x)\
	printf("Loading " #x " ... ");\
	opt = load_sound(str);\
	if (opt) {\
		Sound_Id::x = *opt;\
		printf("sucess :) !\n");\
	}\
	else {\
		printf("failed :( !\n");\
	}
	X("assets/sounds/ui_action.wav",   Ui_Action);
	X("assets/sounds/range1_shot.wav", Range1_Shoot);
#undef X
}

void Store_t::load_from_config(std::filesystem::path config_path) noexcept {
}


[[nodiscard]] Font& Store_t::get_font(size_t k) noexcept {
	return fonts.at(k).asset;
}

[[nodiscard]] bool Store_t::load_font(size_t k, std::filesystem::path path) noexcept {
	Asset_Font font;
	font.path = path;

	auto opt = load_from_json_file(path);
	if (!opt) return false;
	font.asset.info = (Font::Font_Info)*opt;

	auto it = std::find_if(
		BEG_END(textures),
		[p = font.asset.info.texture_path](const auto& x) { return x.second.path == p; }
	);

	if (it == END(textures)) {
		auto opt_key = load_texture(path.parent_path() / font.asset.info.texture_path);
		if (!opt_key) return false;
		font.asset.texture_id = *opt_key;
	} else {
		font.asset.texture_id = it->first;
	}
	
	get_albedo(font.asset.texture_id).set_resize_filter(Texture::Filter::Linear);
	
	fonts.emplace(k, std::move(font));
	
	return true;
}

[[nodiscard]] std::optional<size_t> Store_t::load_font(std::filesystem::path path) noexcept{
	size_t k = xstd::uuid();
	if (auto opt = load_font(k, path)) return k;
	return std::nullopt;
}

[[nodiscard]] DLL& Store_t::get_dll(size_t k) noexcept {
	return dlls.at(k).asset;
}
[[nodiscard]] bool Store_t::load_dll(size_t k, std::filesystem::path path) noexcept {
	auto opt = ::load_dll(path);
	if (!opt) return false;

	Asset_DLL dll;
	dll.asset = std::move(*opt);
	dll.path = std::filesystem::weakly_canonical(path);
	dlls.emplace(k, std::move(dll));
	return true;
}
[[nodiscard]] std::optional<size_t> Store_t::load_dll(std::filesystem::path path) noexcept {
	size_t k = xstd::uuid();
	if (auto opt = load_dll(k, path)) return k;
	return std::nullopt;
}


[[nodiscard]] Object& Store_t::get_object(size_t k) noexcept {
	return objects.at(k).asset;
}
[[nodiscard]] bool Store_t::load_object(size_t k, std::filesystem::path path) noexcept {
	auto opt = Object::load_from_file(path);
	if (!opt) return false;

	Asset_Object obj;
	obj.asset = std::move(*opt);
	obj.path = std::filesystem::weakly_canonical(path);
	objects.emplace(k, std::move(obj));
	return true;
}
[[nodiscard]] std::optional<size_t> Store_t::load_object(std::filesystem::path path) noexcept {
	size_t k = xstd::uuid();
	if (auto opt = load_object(k, path)) return k;
	return std::nullopt;
}


[[nodiscard]] ma_decoder& Store_t::get_sound(size_t k) noexcept {
	return sounds.at(k).asset;
}
[[nodiscard]] bool Store_t::load_sound(size_t k, std::filesystem::path path) noexcept {
	auto& s = sounds[k];
	s.path = path;
	auto res = ma_decoder_init_file(path.generic_string().c_str(), nullptr, &s.asset);
	if (res != MA_SUCCESS) {
		objects.erase(k);
		return false;
	}

	return true;
}
[[nodiscard]] std::optional<size_t> Store_t::load_sound(std::filesystem::path path) noexcept {
	size_t k = xstd::uuid();
	if (auto opt = load_sound(k, path)) return k;
	return std::nullopt;
}
