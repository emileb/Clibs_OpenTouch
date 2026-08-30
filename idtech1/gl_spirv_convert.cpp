/*
** gl_spirv_convert.cpp
**
** Desktop GLSL -> GLSL ES 3.10 for the mobile OpenGL renderer, so mod shaders written for
** desktop GL compile on Android. Pipeline: glslang (GL client, auto-mapped locations/bindings)
** -> SPIR-V -> SPIRV-Cross GLSL ES backend (C API, libspirvcross.so). Shared by uzdoom / gzdoom_dev / gzdoom_4.11.3, compiled into each with its own glslang. Names are kept so GZDoom's by-name uniform/UBO
** lookups keep working.
*/

#include "gl_spirv_convert.h"
#include "printf.h"
#include "glslang/glslang/Public/ShaderLang.h"
#include "glslang/spirv/GlslangToSpv.h"
#include "spirv_cross_c.h"
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cstring>

// glslang numbers in/out per stage by declaration order, which differs between main.vp and main.fp,
// so varying locations are assigned here by name across both stages instead.
struct VaryingLocations
{
	std::map<std::string, uint32_t> byName;
	uint32_t next = 0;
};

// Same table ZVulkan uses (vulkanbuilders.cpp), it is not exported from there
static const TBuiltInResource DefaultTBuiltInResource = {
	/* .MaxLights = */ 32,
	/* .MaxClipPlanes = */ 6,
	/* .MaxTextureUnits = */ 32,
	/* .MaxTextureCoords = */ 32,
	/* .MaxVertexAttribs = */ 64,
	/* .MaxVertexUniformComponents = */ 4096,
	/* .MaxVaryingFloats = */ 64,
	/* .MaxVertexTextureImageUnits = */ 32,
	/* .MaxCombinedTextureImageUnits = */ 80,
	/* .MaxTextureImageUnits = */ 32,
	/* .MaxFragmentUniformComponents = */ 4096,
	/* .MaxDrawBuffers = */ 32,
	/* .MaxVertexUniformVectors = */ 128,
	/* .MaxVaryingVectors = */ 8,
	/* .MaxFragmentUniformVectors = */ 16,
	/* .MaxVertexOutputVectors = */ 16,
	/* .MaxFragmentInputVectors = */ 15,
	/* .MinProgramTexelOffset = */ -8,
	/* .MaxProgramTexelOffset = */ 7,
	/* .MaxClipDistances = */ 8,
	/* .MaxComputeWorkGroupCountX = */ 65535,
	/* .MaxComputeWorkGroupCountY = */ 65535,
	/* .MaxComputeWorkGroupCountZ = */ 65535,
	/* .MaxComputeWorkGroupSizeX = */ 1024,
	/* .MaxComputeWorkGroupSizeY = */ 1024,
	/* .MaxComputeWorkGroupSizeZ = */ 64,
	/* .MaxComputeUniformComponents = */ 1024,
	/* .MaxComputeTextureImageUnits = */ 16,
	/* .MaxComputeImageUniforms = */ 8,
	/* .MaxComputeAtomicCounters = */ 8,
	/* .MaxComputeAtomicCounterBuffers = */ 1,
	/* .MaxVaryingComponents = */ 60,
	/* .MaxVertexOutputComponents = */ 64,
	/* .MaxGeometryInputComponents = */ 64,
	/* .MaxGeometryOutputComponents = */ 128,
	/* .MaxFragmentInputComponents = */ 128,
	/* .MaxImageUnits = */ 8,
	/* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
	/* .MaxCombinedShaderOutputResources = */ 8,
	/* .MaxImageSamples = */ 0,
	/* .MaxVertexImageUniforms = */ 0,
	/* .MaxTessControlImageUniforms = */ 0,
	/* .MaxTessEvaluationImageUniforms = */ 0,
	/* .MaxGeometryImageUniforms = */ 0,
	/* .MaxFragmentImageUniforms = */ 8,
	/* .MaxCombinedImageUniforms = */ 8,
	/* .MaxGeometryTextureImageUnits = */ 16,
	/* .MaxGeometryOutputVertices = */ 256,
	/* .MaxGeometryTotalOutputComponents = */ 1024,
	/* .MaxGeometryUniformComponents = */ 1024,
	/* .MaxGeometryVaryingComponents = */ 64,
	/* .MaxTessControlInputComponents = */ 128,
	/* .MaxTessControlOutputComponents = */ 128,
	/* .MaxTessControlTextureImageUnits = */ 16,
	/* .MaxTessControlUniformComponents = */ 1024,
	/* .MaxTessControlTotalOutputComponents = */ 4096,
	/* .MaxTessEvaluationInputComponents = */ 128,
	/* .MaxTessEvaluationOutputComponents = */ 128,
	/* .MaxTessEvaluationTextureImageUnits = */ 16,
	/* .MaxTessEvaluationUniformComponents = */ 1024,
	/* .MaxTessPatchComponents = */ 120,
	/* .MaxPatchVertices = */ 32,
	/* .MaxTessGenLevel = */ 64,
	/* .MaxViewports = */ 16,
	/* .MaxVertexAtomicCounters = */ 0,
	/* .MaxTessControlAtomicCounters = */ 0,
	/* .MaxTessEvaluationAtomicCounters = */ 0,
	/* .MaxGeometryAtomicCounters = */ 0,
	/* .MaxFragmentAtomicCounters = */ 8,
	/* .MaxCombinedAtomicCounters = */ 8,
	/* .MaxAtomicCounterBindings = */ 1,
	/* .MaxVertexAtomicCounterBuffers = */ 0,
	/* .MaxTessControlAtomicCounterBuffers = */ 0,
	/* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
	/* .MaxGeometryAtomicCounterBuffers = */ 0,
	/* .MaxFragmentAtomicCounterBuffers = */ 1,
	/* .MaxCombinedAtomicCounterBuffers = */ 1,
	/* .MaxAtomicCounterBufferSize = */ 16384,
	/* .MaxTransformFeedbackBuffers = */ 4,
	/* .MaxTransformFeedbackInterleavedComponents = */ 64,
	/* .MaxCullDistances = */ 8,
	/* .MaxCombinedClipAndCullDistances = */ 8,
	/* .MaxSamples = */ 4,
	/* .maxMeshOutputVerticesNV = */ 256,
	/* .maxMeshOutputPrimitivesNV = */ 512,
	/* .maxMeshWorkGroupSizeX_NV = */ 32,
	/* .maxMeshWorkGroupSizeY_NV = */ 1,
	/* .maxMeshWorkGroupSizeZ_NV = */ 1,
	/* .maxTaskWorkGroupSizeX_NV = */ 32,
	/* .maxTaskWorkGroupSizeY_NV = */ 1,
	/* .maxTaskWorkGroupSizeZ_NV = */ 1,
	/* .maxMeshViewCountNV = */ 4,
	/* .maxDualSourceDrawBuffersEXT = */ 1,

	/* .limits = */ {
		/* .nonInductiveForLoops = */ 1,
		/* .whileLoops = */ 1,
		/* .doWhileLoops = */ 1,
		/* .generalUniformIndexing = */ 1,
		/* .generalAttributeMatrixVectorIndexing = */ 1,
		/* .generalVaryingIndexing = */ 1,
		/* .generalSamplerIndexing = */ 1,
		/* .generalVariableIndexing = */ 1,
		/* .generalConstantMatrixVectorIndexing = */ 1,
	}
};

// The renderer composes mobile sources with a GLES header; retarget it so glslang parses desktop GLSL.
// precision statements are GLES only and SPIRV-Cross regenerates them, so drop them.
static std::string RetargetToDesktop(const FString &src)
{
	std::string s = src.GetChars();
	static const char esHeader[] = "#version 310 es";
	if (s.compare(0, sizeof(esHeader) - 1, esHeader) == 0)
		s.replace(0, sizeof(esHeader) - 1, "#version 410");

	std::string out;
	out.reserve(s.size());
	size_t pos = 0;
	while (pos < s.size())
	{
		size_t eol = s.find('\n', pos);
		if (eol == std::string::npos) eol = s.size() - 1;
		size_t b = pos;
		while (b < eol && (s[b] == ' ' || s[b] == '\t')) b++;
		if (s.compare(b, 10, "precision ") != 0)
			out.append(s, pos, eol - pos + 1);
		pos = eol + 1;
	}
	return out;
}

static bool ParseStage(glslang::TShader &shader, EShLanguage stage, const std::string &src, const char *stageName, FString &error)
{
	const char *strs[1] = { src.c_str() };
	shader.setStrings(strs, 1);
	shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientOpenGL, 450);
	shader.setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
	shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
	// SPIR-V for GL needs locations/bindings on everything; let glslang assign them.
	// The bindings are numbered per stage and are stripped again in EmitStage (GZDoom binds by name).
	shader.setAutoMapLocations(true);
	shader.setAutoMapBindings(true);

	if (!shader.parse(&DefaultTBuiltInResource, 410, false, EShMsgSpvRules))
	{
		error << stageName << " (glslang):\n" << shader.getInfoLog() << "\n";
		return false;
	}
	return true;
}

static bool EmitStage(glslang::TProgram &program, EShLanguage stage, const char *stageName, VaryingLocations &varyings, FString &out, FString &error)
{
	glslang::SpvOptions opts;
	opts.generateDebugInfo = false;
	opts.stripDebugInfo = false; // keep OpName so uniform/block names survive
	opts.disableOptimizer = true;
	opts.validate = false;

	std::vector<unsigned int> spirv;
	spv::SpvBuildLogger logger;
	glslang::GlslangToSpv(*program.getIntermediate(stage), spirv, &logger, &opts);

	// SPIRV-Cross lives in libspirvcross.so; the C API keeps its exceptions on that side
	spvc_context ctx = nullptr;
	if (spvc_context_create(&ctx) != SPVC_SUCCESS)
	{
		error << stageName << " (SPIRV-Cross): context creation failed\n";
		return false;
	}

	bool ok = false;
	auto fail = [&](const char *what)
	{
		error << stageName << " (SPIRV-Cross " << what << "):\n" << spvc_context_get_last_error_string(ctx) << "\n";
	};

	do
	{
		spvc_parsed_ir ir = nullptr;
		spvc_compiler comp = nullptr;
		spvc_resources res = nullptr;
		if (spvc_context_parse_spirv(ctx, spirv.data(), spirv.size(), &ir) != SPVC_SUCCESS) { fail("parse"); break; }
		if (spvc_context_create_compiler(ctx, SPVC_BACKEND_GLSL, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &comp) != SPVC_SUCCESS) { fail("compiler"); break; }
		if (spvc_compiler_create_shader_resources(comp, &res) != SPVC_SUCCESS) { fail("resources"); break; }

		// GLES rejects bindings on loose uniforms, and per-stage numbering mismatches at link for the rest
		const spvc_resource_type bindingTypes[] = {
			SPVC_RESOURCE_TYPE_GL_PLAIN_UNIFORM, SPVC_RESOURCE_TYPE_UNIFORM_BUFFER, SPVC_RESOURCE_TYPE_SAMPLED_IMAGE,
			SPVC_RESOURCE_TYPE_SEPARATE_IMAGE, SPVC_RESOURCE_TYPE_STORAGE_BUFFER };
		for (auto type : bindingTypes)
		{
			const spvc_reflected_resource *list = nullptr;
			size_t count = 0;
			if (spvc_resources_get_resource_list_for_type(res, type, &list, &count) != SPVC_SUCCESS) continue;
			for (size_t i = 0; i < count; i++)
				spvc_compiler_unset_decoration(comp, list[i].id, SpvDecorationBinding);
		}

		const spvc_reflected_resource *io = nullptr;
		size_t ioCount = 0;
		if (spvc_resources_get_resource_list_for_type(res, stage == EShLangVertex ? SPVC_RESOURCE_TYPE_STAGE_OUTPUT : SPVC_RESOURCE_TYPE_STAGE_INPUT, &io, &ioCount) != SPVC_SUCCESS) { fail("stage io"); break; }
		for (size_t i = 0; i < ioCount; i++)
		{
			auto it = varyings.byName.find(io[i].name);
			uint32_t loc;
			if (it != varyings.byName.end())
			{
				loc = it->second;
			}
			else
			{
				loc = varyings.next;
				spvc_type type = spvc_compiler_get_type_handle(comp, io[i].type_id);
				uint32_t slots = spvc_type_get_columns(type);
				unsigned dims = spvc_type_get_num_array_dimensions(type);
				for (unsigned d = 0; d < dims; d++)
					slots *= spvc_type_array_dimension_is_literal(type, d) ? std::max(1u, (unsigned)spvc_type_get_array_dimension(type, d)) : 1;
				varyings.next += slots;
				varyings.byName[io[i].name] = loc;
			}
			spvc_compiler_set_decoration(comp, io[i].id, SpvDecorationLocation, loc);
		}

		spvc_compiler_options copts = nullptr;
		if (spvc_compiler_create_compiler_options(comp, &copts) != SPVC_SUCCESS) { fail("options"); break; }
		spvc_compiler_options_set_uint(copts, SPVC_COMPILER_OPTION_GLSL_VERSION, 310);
		spvc_compiler_options_set_bool(copts, SPVC_COMPILER_OPTION_GLSL_ES, SPVC_TRUE);
		spvc_compiler_options_set_bool(copts, SPVC_COMPILER_OPTION_GLSL_VULKAN_SEMANTICS, SPVC_FALSE);
		spvc_compiler_options_set_bool(copts, SPVC_COMPILER_OPTION_GLSL_ENABLE_420PACK_EXTENSION, SPVC_FALSE);
		// mediump defaults blow out world-space lighting on mobile GPUs
		spvc_compiler_options_set_bool(copts, SPVC_COMPILER_OPTION_GLSL_ES_DEFAULT_FLOAT_PRECISION_HIGHP, SPVC_TRUE);
		spvc_compiler_options_set_bool(copts, SPVC_COMPILER_OPTION_GLSL_ES_DEFAULT_INT_PRECISION_HIGHP, SPVC_TRUE);
		if (spvc_compiler_install_compiler_options(comp, copts) != SPVC_SUCCESS) { fail("install options"); break; }

		const char *result = nullptr;
		if (spvc_compiler_compile(comp, &result) != SPVC_SUCCESS) { fail("compile"); break; }
		out = result;
		ok = true;
	} while (false);

	spvc_context_destroy(ctx);
	return ok;
}

bool GL_ConvertProgramToGLES(const char *name, FString &vertSrc, FString &fragSrc, FString &error)
{
	static bool initialized = false;
	if (!initialized)
	{
		glslang::InitializeProcess();
		initialized = true;
	}

	std::string vsrc = RetargetToDesktop(vertSrc);
	std::string fsrc = RetargetToDesktop(fragSrc);

	glslang::TShader vs(EShLangVertex), fs(EShLangFragment);
	if (!ParseStage(vs, EShLangVertex, vsrc, "Vertex shader", error)) return false;
	if (!ParseStage(fs, EShLangFragment, fsrc, "Fragment shader", error)) return false;

	glslang::TProgram program;
	program.addShader(&vs);
	program.addShader(&fs);
	if (!program.link(EShMsgSpvRules) || !program.mapIO())
	{
		error << "Linking (glslang):\n" << program.getInfoLog() << "\n";
		return false;
	}

	FString vout, fout;
	VaryingLocations varyings;
	if (!EmitStage(program, EShLangVertex, "Vertex shader", varyings, vout, error)) return false;
	if (!EmitStage(program, EShLangFragment, "Fragment shader", varyings, fout, error)) return false;

	DPrintf(DMSG_NOTIFY, "SPIRV-Cross converted shader '%s'\n", name);
	vertSrc = vout;
	fragSrc = fout;
	return true;
}
