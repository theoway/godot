#ifndef RASTERIZERSTORAGEGLES3_H
#define RASTERIZERSTORAGEGLES3_H

#include "servers/visual/rasterizer.h"
#include "servers/visual/shader_language.h"
#include "shader_gles3.h"
#include "shaders/copy.glsl.h"
#include "shaders/canvas.glsl.h"
#include "shaders/cubemap_filter.glsl.h"
#include "self_list.h"
#include "shader_compiler_gles3.h"

class RasterizerCanvasGLES3;
class RasterizerSceneGLES3;

#define _TEXTURE_SRGB_DECODE_EXT        0x8A48
#define _DECODE_EXT             0x8A49
#define _SKIP_DECODE_EXT        0x8A4A

class RasterizerStorageGLES3 : public RasterizerStorage {
public:

	RasterizerCanvasGLES3 *canvas;
	RasterizerSceneGLES3 *scene;

	enum RenderArchitecture {
		RENDER_ARCH_MOBILE,
		RENDER_ARCH_DESKTOP,
	};

	struct Config {

		RenderArchitecture render_arch;

		GLuint system_fbo; //on some devices, such as apple, screen is rendered to yet another fbo.

		bool shrink_textures_x2;
		bool use_fast_texture_filter;
		bool use_anisotropic_filter;

		bool s3tc_supported;
		bool latc_supported;
		bool bptc_supported;
		bool etc_supported;
		bool etc2_supported;
		bool pvrtc_supported;

		bool srgb_decode_supported;

		bool use_rgba_2d_shadows;

		float anisotropic_level;

		int max_texture_image_units;
		int max_texture_size;

		Set<String> extensions;
	} config;

	mutable struct Shaders {

		CopyShaderGLES3 copy;

		ShaderCompilerGLES3 compiler;

		CubemapFilterShaderGLES3 cubemap_filter;

		ShaderCompilerGLES3::IdentifierActions actions_canvas;
		ShaderCompilerGLES3::IdentifierActions actions_scene;
	} shaders;

	struct Resources {

		GLuint white_tex;
		GLuint black_tex;
		GLuint normal_tex;

		GLuint quadie;
		GLuint quadie_array;

	} resources;

	struct Info {

		uint64_t texture_mem;

	} info;


/////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////DATA///////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////









	struct Instantiable : public RID_Data {

		SelfList<RasterizerScene::InstanceBase>::List instance_list;

		_FORCE_INLINE_ void instance_change_notify() {

			SelfList<RasterizerScene::InstanceBase> *instances = instance_list.first();
			while(instances) {

				instances->self()->base_changed();
				instances=instances->next();
			}
		}

		_FORCE_INLINE_ void instance_material_change_notify() {

			SelfList<RasterizerScene::InstanceBase> *instances = instance_list.first();
			while(instances) {

				instances->self()->base_material_changed();
				instances=instances->next();
			}
		}

		Instantiable() {  }
		virtual ~Instantiable() {

			while(instance_list.first()) {
				instance_list.first()->self()->base_removed();
			}
		}
	};






/////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////API////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////


	/* TEXTURE API */

	struct RenderTarget;

	struct Texture : public RID_Data {

		String path;
		uint32_t flags;
		int width,height;
		int alloc_width, alloc_height;
		Image::Format format;

		GLenum target;
		GLenum gl_format_cache;
		GLenum gl_internal_format_cache;
		GLenum gl_type_cache;
		int data_size; //original data size, useful for retrieving back
		bool compressed;
		bool srgb;
		int total_data_size;
		bool ignore_mipmaps;

		int mipmaps;

		bool active;
		GLuint tex_id;

		bool using_srgb;

		uint16_t stored_cube_sides;

		RenderTarget *render_target;

		Texture() {

			using_srgb=false;
			stored_cube_sides=0;
			ignore_mipmaps=false;
			render_target=NULL;
			flags=width=height=0;
			tex_id=0;
			data_size=0;
			format=Image::FORMAT_L8;
			active=false;
			compressed=false;
			total_data_size=0;
			target=GL_TEXTURE_2D;
			mipmaps=0;

		}

		~Texture() {

			if (tex_id!=0) {

				glDeleteTextures(1,&tex_id);
			}
		}
	};

	mutable RID_Owner<Texture> texture_owner;

	Image _get_gl_image_and_format(const Image& p_image, Image::Format p_format, uint32_t p_flags, GLenum& r_gl_format, GLenum& r_gl_internal_format, GLenum &r_type, bool &r_compressed, bool &srgb);

	virtual RID texture_create();
	virtual void texture_allocate(RID p_texture,int p_width, int p_height,Image::Format p_format,uint32_t p_flags=VS::TEXTURE_FLAGS_DEFAULT);
	virtual void texture_set_data(RID p_texture,const Image& p_image,VS::CubeMapSide p_cube_side=VS::CUBEMAP_LEFT);
	virtual Image texture_get_data(RID p_texture,VS::CubeMapSide p_cube_side=VS::CUBEMAP_LEFT) const;
	virtual void texture_set_flags(RID p_texture,uint32_t p_flags);
	virtual uint32_t texture_get_flags(RID p_texture) const;
	virtual Image::Format texture_get_format(RID p_texture) const;
	virtual uint32_t texture_get_width(RID p_texture) const;
	virtual uint32_t texture_get_height(RID p_texture) const;
	virtual void texture_set_size_override(RID p_texture,int p_width, int p_height);

	virtual void texture_set_path(RID p_texture,const String& p_path);
	virtual String texture_get_path(RID p_texture) const;

	virtual void texture_set_shrink_all_x2_on_set_data(bool p_enable);

	virtual void texture_debug_usage(List<VS::TextureInfo> *r_info);

	virtual RID texture_create_radiance_cubemap(RID p_source,int p_resolution=-1) const;


	/* SHADER API */

	struct Material;

	struct Shader : public RID_Data {

		RID self;

		VS::ShaderMode mode;
		ShaderGLES3 *shader;
		String code;
		SelfList<Material>::List materials;



		Map<StringName,ShaderLanguage::ShaderNode::Uniform> uniforms;
		Vector<uint32_t> ubo_offsets;
		uint32_t ubo_size;

		uint32_t texture_count;

		uint32_t custom_code_id;
		uint32_t version;

		SelfList<Shader> dirty_list;

		Map<StringName,RID> default_textures;

		Vector<ShaderLanguage::ShaderNode::Uniform::Hint> texture_hints;

		bool valid;

		String path;

		struct CanvasItem {

			enum BlendMode {
				BLEND_MODE_MIX,
				BLEND_MODE_ADD,
				BLEND_MODE_SUB,
				BLEND_MODE_MUL,
				BLEND_MODE_PMALPHA,
			};

			int blend_mode;

			enum LightMode {
				LIGHT_MODE_NORMAL,
				LIGHT_MODE_UNSHADED,
				LIGHT_MODE_LIGHT_ONLY
			};

			int light_mode;

		} canvas_item;

		struct Spatial {

			enum BlendMode {
				BLEND_MODE_MIX,
				BLEND_MODE_ADD,
				BLEND_MODE_SUB,
				BLEND_MODE_MUL,
			};

			int blend_mode;

			enum DepthDrawMode {
				DEPTH_DRAW_OPAQUE,
				DEPTH_DRAW_ALWAYS,
				DEPTH_DRAW_NEVER,
				DEPTH_DRAW_ALPHA_PREPASS,
			};

			int depth_draw_mode;

			enum CullMode {
				CULL_MODE_FRONT,
				CULL_MODE_BACK,
				CULL_MODE_DISABLED,
			};

			int cull_mode;

			bool uses_alpha;
			bool unshaded;
			bool ontop;
			bool uses_vertex;
			bool uses_discard;

		} spatial;

		bool uses_vertex_time;
		bool uses_fragment_time;

		Shader() : dirty_list(this) {

			shader=NULL;
			valid=false;
			custom_code_id=0;
			version=1;
		}
	};

	mutable SelfList<Shader>::List _shader_dirty_list;
	void _shader_make_dirty(Shader* p_shader);

	mutable RID_Owner<Shader> shader_owner;

	virtual RID shader_create(VS::ShaderMode p_mode=VS::SHADER_SPATIAL);

	virtual void shader_set_mode(RID p_shader,VS::ShaderMode p_mode);
	virtual VS::ShaderMode shader_get_mode(RID p_shader) const;

	virtual void shader_set_code(RID p_shader, const String& p_code);
	virtual String shader_get_code(RID p_shader) const;
	virtual void shader_get_param_list(RID p_shader, List<PropertyInfo> *p_param_list) const;

	virtual void shader_set_default_texture_param(RID p_shader, const StringName& p_name, RID p_texture);
	virtual RID shader_get_default_texture_param(RID p_shader, const StringName& p_name) const;

	void _update_shader(Shader* p_shader) const;

	void update_dirty_shaders();



	/* COMMON MATERIAL API */

	struct Material : public RID_Data {

		Shader *shader;
		GLuint ubo_id;
		uint32_t ubo_size;
		Map<StringName,Variant> params;
		SelfList<Material> list;
		SelfList<Material> dirty_list;
		Vector<RID> textures;
		float line_width;

		uint32_t index;
		uint64_t last_pass;

		Map<Instantiable*,int> instantiable_owners;
		Map<RasterizerScene::InstanceBase*,int> instance_owners;

		bool can_cast_shadow_cache;
		bool is_animated_cache;

		Material() : list(this), dirty_list(this) {
			can_cast_shadow_cache=false;
			is_animated_cache=false;
			shader=NULL;
			line_width=1.0;
			ubo_id=0;
			ubo_size=0;
			last_pass=0;
		}

	};

	mutable SelfList<Material>::List _material_dirty_list;
	void _material_make_dirty(Material *p_material) const;
	void _material_add_instantiable(RID p_material,Instantiable *p_instantiable);
	void _material_remove_instantiable(RID p_material, Instantiable *p_instantiable);


	mutable RID_Owner<Material> material_owner;

	virtual RID material_create();

	virtual void material_set_shader(RID p_material, RID p_shader);
	virtual RID material_get_shader(RID p_material) const;

	virtual void material_set_param(RID p_material, const StringName& p_param, const Variant& p_value);
	virtual Variant material_get_param(RID p_material, const StringName& p_param) const;

	virtual void material_set_line_width(RID p_material, float p_width);

	virtual bool material_is_animated(RID p_material);
	virtual bool material_casts_shadows(RID p_material);

	virtual void material_add_instance_owner(RID p_material, RasterizerScene::InstanceBase *p_instance);
	virtual void material_remove_instance_owner(RID p_material, RasterizerScene::InstanceBase *p_instance);

	void _update_material(Material* material);

	void update_dirty_materials();

	/* MESH API */


	struct Geometry : Instantiable {

		enum Type {
			GEOMETRY_INVALID,
			GEOMETRY_SURFACE,
			GEOMETRY_IMMEDIATE,
			GEOMETRY_MULTISURFACE,
		};

		Type type;
		RID material;
		uint64_t last_pass;
		uint32_t index;

		Geometry() {
			last_pass=0;
			index=0;
		}

	};

	struct GeometryOwner : public Instantiable {

		virtual ~GeometryOwner() {}
	};

	struct Mesh;
	struct Surface : public Geometry {

		struct Attrib {

			bool enabled;
			GLuint index;
			GLint size;
			GLenum type;
			GLboolean normalized;
			GLsizei stride;
			uint32_t offset;
		};

		Attrib attribs[VS::ARRAY_MAX];
		Attrib morph_attribs[VS::ARRAY_MAX];


		Mesh *mesh;
		uint32_t format;

		GLuint array_id;
		GLuint vertex_id;
		GLuint index_id;

		Vector<AABB> skeleton_bone_aabb;
		Vector<bool> skeleton_bone_used;

		//bool packed;

		struct MorphTarget {
			GLuint vertex_id;
			GLuint array_id;
		};

		Vector<MorphTarget> morph_targets;

		AABB aabb;

		int array_len;
		int index_array_len;
		int max_bone;

		int array_byte_size;
		int index_array_byte_size;


		VS::PrimitiveType primitive;

		bool active;

		Surface() {

			array_byte_size=0;
			index_array_byte_size=0;
			mesh=NULL;
			format=0;
			array_id=0;
			vertex_id=0;
			index_id=0;
			array_len=0;
			type=GEOMETRY_SURFACE;
			primitive=VS::PRIMITIVE_POINTS;
			index_array_len=0;
			active=false;

		}

		~Surface() {

		}
	};


	struct Mesh : public GeometryOwner {

		bool active;
		Vector<Surface*> surfaces;
		int morph_target_count;
		VS::MorphTargetMode morph_target_mode;
		AABB custom_aabb;
		mutable uint64_t last_pass;
		Mesh() {
			morph_target_mode=VS::MORPH_MODE_NORMALIZED;
			morph_target_count=0;
			last_pass=0;
			active=false;
		}
	};

	mutable RID_Owner<Mesh> mesh_owner;

	virtual RID mesh_create();

	virtual void mesh_add_surface(RID p_mesh,uint32_t p_format,VS::PrimitiveType p_primitive,const DVector<uint8_t>& p_array,int p_vertex_count,const DVector<uint8_t>& p_index_array,int p_index_count,const AABB& p_aabb,const Vector<DVector<uint8_t> >& p_blend_shapes=Vector<DVector<uint8_t> >(),const Vector<AABB>& p_bone_aabbs=Vector<AABB>());

	virtual void mesh_set_morph_target_count(RID p_mesh,int p_amount);
	virtual int mesh_get_morph_target_count(RID p_mesh) const;


	virtual void mesh_set_morph_target_mode(RID p_mesh,VS::MorphTargetMode p_mode);
	virtual VS::MorphTargetMode mesh_get_morph_target_mode(RID p_mesh) const;

	virtual void mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material);
	virtual RID mesh_surface_get_material(RID p_mesh, int p_surface) const;

	virtual int mesh_surface_get_array_len(RID p_mesh, int p_surface) const;
	virtual int mesh_surface_get_array_index_len(RID p_mesh, int p_surface) const;

	virtual DVector<uint8_t> mesh_surface_get_array(RID p_mesh, int p_surface) const;
	virtual DVector<uint8_t> mesh_surface_get_index_array(RID p_mesh, int p_surface) const;


	virtual uint32_t mesh_surface_get_format(RID p_mesh, int p_surface) const;
	virtual VS::PrimitiveType mesh_surface_get_primitive_type(RID p_mesh, int p_surface) const;

	virtual AABB mesh_surface_get_aabb(RID p_mesh, int p_surface) const;
	virtual Vector<DVector<uint8_t> > mesh_surface_get_blend_shapes(RID p_mesh, int p_surface) const;
	virtual Vector<AABB> mesh_surface_get_skeleton_aabb(RID p_mesh, int p_surface) const;

	virtual void mesh_remove_surface(RID p_mesh, int p_surface);
	virtual int mesh_get_surface_count(RID p_mesh) const;

	virtual void mesh_set_custom_aabb(RID p_mesh,const AABB& p_aabb);
	virtual AABB mesh_get_custom_aabb(RID p_mesh) const;

	virtual AABB mesh_get_aabb(RID p_mesh, RID p_skeleton) const;
	virtual void mesh_clear(RID p_mesh);

	/* MULTIMESH API */


	virtual RID multimesh_create();

	virtual void multimesh_allocate(RID p_multimesh,int p_instances,VS::MultimeshTransformFormat p_transform_format,VS::MultimeshColorFormat p_color_format,bool p_gen_aabb=true);
	virtual int multimesh_get_instance_count(RID p_multimesh) const;

	virtual void multimesh_set_mesh(RID p_multimesh,RID p_mesh);
	virtual void multimesh_set_custom_aabb(RID p_multimesh,const AABB& p_aabb);
	virtual void multimesh_instance_set_transform(RID p_multimesh,int p_index,const Transform& p_transform);
	virtual void multimesh_instance_set_transform_2d(RID p_multimesh,int p_index,const Matrix32& p_transform);
	virtual void multimesh_instance_set_color(RID p_multimesh,int p_index,const Color& p_color);

	virtual RID multimesh_get_mesh(RID p_multimesh) const;
	virtual AABB multimesh_get_custom_aabb(RID p_multimesh) const;

	virtual Transform multimesh_instance_get_transform(RID p_multimesh,int p_index) const;
	virtual Matrix32 multimesh_instance_get_transform_2d(RID p_multimesh,int p_index) const;
	virtual Color multimesh_instance_get_color(RID p_multimesh,int p_index) const;

	virtual void multimesh_set_visible_instances(RID p_multimesh,int p_visible);
	virtual int multimesh_get_visible_instances(RID p_multimesh) const;

	virtual AABB multimesh_get_aabb(RID p_mesh) const;

	/* IMMEDIATE API */

	virtual RID immediate_create();
	virtual void immediate_begin(RID p_immediate,VS::PrimitiveType p_rimitive,RID p_texture=RID());
	virtual void immediate_vertex(RID p_immediate,const Vector3& p_vertex);
	virtual void immediate_vertex_2d(RID p_immediate,const Vector3& p_vertex);
	virtual void immediate_normal(RID p_immediate,const Vector3& p_normal);
	virtual void immediate_tangent(RID p_immediate,const Plane& p_tangent);
	virtual void immediate_color(RID p_immediate,const Color& p_color);
	virtual void immediate_uv(RID p_immediate,const Vector2& tex_uv);
	virtual void immediate_uv2(RID p_immediate,const Vector2& tex_uv);
	virtual void immediate_end(RID p_immediate);
	virtual void immediate_clear(RID p_immediate);
	virtual void immediate_set_material(RID p_immediate,RID p_material);
	virtual RID immediate_get_material(RID p_immediate) const;

	/* SKELETON API */

	virtual RID skeleton_create();
	virtual void skeleton_allocate(RID p_skeleton,int p_bones,bool p_2d_skeleton=false);
	virtual int skeleton_get_bone_count(RID p_skeleton) const;
	virtual void skeleton_bone_set_transform(RID p_skeleton,int p_bone, const Transform& p_transform);
	virtual Transform skeleton_bone_get_transform(RID p_skeleton,int p_bone) const;
	virtual void skeleton_bone_set_transform_2d(RID p_skeleton,int p_bone, const Matrix32& p_transform);
	virtual Matrix32 skeleton_bone_get_transform_2d(RID p_skeleton,int p_bone) const;

	/* Light API */


	struct Light : Instantiable {

		VS::LightType type;
		float param[VS::LIGHT_PARAM_MAX];
		Color color;
		bool shadow;
		bool negative;
		uint32_t cull_mask;
		VS::LightOmniShadowMode omni_shadow_mode;
		VS::LightOmniShadowDetail omni_shadow_detail;
		VS::LightDirectionalShadowMode directional_shadow_mode;
		uint64_t version;
	};

	mutable RID_Owner<Light> light_owner;

	virtual RID light_create(VS::LightType p_type);

	virtual void light_set_color(RID p_light,const Color& p_color);
	virtual void light_set_param(RID p_light,VS::LightParam p_param,float p_value);
	virtual void light_set_shadow(RID p_light,bool p_enabled);
	virtual void light_set_projector(RID p_light,RID p_texture);
	virtual void light_set_attenuation_texure(RID p_light,RID p_texture);
	virtual void light_set_negative(RID p_light,bool p_enable);
	virtual void light_set_cull_mask(RID p_light,uint32_t p_mask);
	virtual void light_set_shader(RID p_light,RID p_shader);

	virtual void light_omni_set_shadow_mode(RID p_light,VS::LightOmniShadowMode p_mode);

	virtual void light_omni_set_shadow_detail(RID p_light,VS::LightOmniShadowDetail p_detail);

	virtual void light_directional_set_shadow_mode(RID p_light,VS::LightDirectionalShadowMode p_mode);
	virtual VS::LightDirectionalShadowMode light_directional_get_shadow_mode(RID p_light);
	virtual VS::LightOmniShadowMode light_omni_get_shadow_mode(RID p_light);

	virtual bool light_has_shadow(RID p_light) const;

	virtual VS::LightType light_get_type(RID p_light) const;
	virtual float light_get_param(RID p_light,VS::LightParam p_param);

	virtual AABB light_get_aabb(RID p_light) const;
	virtual uint64_t light_get_version(RID p_light) const;

	/* PROBE API */

	virtual RID reflection_probe_create();

	virtual void reflection_probe_set_intensity(RID p_probe, float p_intensity);
	virtual void reflection_probe_set_clip(RID p_probe, float p_near, float p_far);
	virtual void reflection_probe_set_min_blend_distance(RID p_probe, float p_distance);
	virtual void reflection_probe_set_extents(RID p_probe, const Vector3& p_extents);
	virtual void reflection_probe_set_origin_offset(RID p_probe, const Vector3& p_offset);
	virtual void reflection_probe_set_enable_parallax_correction(RID p_probe, bool p_enable);
	virtual void reflection_probe_set_resolution(RID p_probe, int p_resolution);
	virtual void reflection_probe_set_hide_skybox(RID p_probe, bool p_hide);
	virtual void reflection_probe_set_cull_mask(RID p_probe, uint32_t p_layers);


	/* ROOM API */

	virtual RID room_create();
	virtual void room_add_bounds(RID p_room, const DVector<Vector2>& p_convex_polygon,float p_height,const Transform& p_transform);
	virtual void room_clear_bounds(RID p_room);

	/* PORTAL API */

	// portals are only (x/y) points, forming a convex shape, which its clockwise
	// order points outside. (z is 0);

	virtual RID portal_create();
	virtual void portal_set_shape(RID p_portal, const Vector<Point2>& p_shape);
	virtual void portal_set_enabled(RID p_portal, bool p_enabled);
	virtual void portal_set_disable_distance(RID p_portal, float p_distance);
	virtual void portal_set_disabled_color(RID p_portal, const Color& p_color);


	virtual void instance_add_dependency(RID p_base,RasterizerScene::InstanceBase *p_instance);
	virtual void instance_remove_dependency(RID p_base,RasterizerScene::InstanceBase *p_instance);

	/* RENDER TARGET */

	struct RenderTarget : public RID_Data {

		struct Color {
			GLuint fbo;
			GLuint color;
		} front,back;

		GLuint depth;

		struct Buffers {
			GLuint fbo;
			GLuint alpha_fbo; //single buffer, just diffuse (for alpha pass)
			GLuint specular;
			GLuint diffuse;
			GLuint normal_sr;
		} buffers;

		int width,height;

		bool flags[RENDER_TARGET_FLAG_MAX];

		bool used_in_frame;

		RID texture;

		RenderTarget() {

			width=0;
			height=0;
			depth=0;
			front.fbo=0;
			back.fbo=0;
			buffers.fbo=0;
			buffers.alpha_fbo=0;
			used_in_frame=false;

			flags[RENDER_TARGET_VFLIP]=false;
			flags[RENDER_TARGET_TRANSPARENT]=false;
			flags[RENDER_TARGET_NO_3D]=false;
			flags[RENDER_TARGET_NO_SAMPLING]=false;
		}
	};

	mutable RID_Owner<RenderTarget> render_target_owner;

	void _render_target_clear(RenderTarget *rt);
	void _render_target_allocate(RenderTarget *rt);

	virtual RID render_target_create();
	virtual void render_target_set_size(RID p_render_target,int p_width, int p_height);
	virtual RID render_target_get_texture(RID p_render_target) const;

	virtual void render_target_set_flag(RID p_render_target,RenderTargetFlags p_flag,bool p_value);
	virtual bool render_target_renedered_in_frame(RID p_render_target);

	/* CANVAS SHADOW */

	struct CanvasLightShadow : public RID_Data {

		int size;
		int height;
		GLuint fbo;
		GLuint depth;
		GLuint distance; //for older devices
	};

	RID_Owner<CanvasLightShadow> canvas_light_shadow_owner;

	virtual RID canvas_light_shadow_buffer_create(int p_width);

	/* LIGHT SHADOW MAPPING */

	struct CanvasOccluder : public RID_Data {

		GLuint vertex_id; // 0 means, unconfigured
		GLuint index_id; // 0 means, unconfigured
		DVector<Vector2> lines;
		int len;
	};

	RID_Owner<CanvasOccluder> canvas_occluder_owner;

	virtual RID canvas_light_occluder_create();
	virtual void canvas_light_occluder_set_polylines(RID p_occluder, const DVector<Vector2>& p_lines);

	virtual VS::InstanceType get_base_type(RID p_rid) const;

	virtual bool free(RID p_rid);


	struct Frame {

		RenderTarget *current_rt;

		bool clear_request;
		Color clear_request_color;
		int canvas_draw_commands;
		float time[4];
	} frame;

	void initialize();
	void finalize();



	RasterizerStorageGLES3();
};


#endif // RASTERIZERSTORAGEGLES3_H
