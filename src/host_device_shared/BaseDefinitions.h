//
// Created by HUSTLX on 2025/1/15.
//

#ifndef BASEDEFINITIONS_H
#define BASEDEFINITIONS_H

#ifdef __cplusplus
#include "glm/glm.hpp"
#include <cmath>
using float4 = glm::vec4;
using float3 = glm::vec3;
using float2 = glm::vec2;
using float4x4 = glm::mat4;
using float3x3 = glm::mat3;
using float4x3 = glm::mat4x3;
using float3x4 = glm::mat3x4;
using uint = uint32_t;
#define PI M_PI
#else
#define PI 3.1415926
#endif
#define M_E 2.71828182846

#ifdef __cplusplus
#define IS_COMPILING_CPP 1
#else
#define IS_COMPILING_CPP 0
#endif

// NOTE: USING_DXC is defined in CompilerHLSL.cpp
#ifdef USING_DXC
#define IS_COMPILING_SHADER 1
#else
#define IS_COMPILING_SHADER 0
#endif


#if IS_COMPILING_CPP
#define DECLARE_MEMBER_WITH_DEFAULT_VALUE(Type, Name, Value) Type Name = Value
#else
#define DECLARE_MEMBER_WITH_DEFAULT_VALUE(Type, Name, Value) Type Name
#endif


#if IS_COMPILING_CPP
#define BEGIN_SHADOWY_NAMESPACE namespace Shadowy {
#define END_SHADOWY_NAME_SPACE }  // namespace Shadowy
#else
#define BEGIN_SHADOWY_NAMESPACE
#define END_SHADOWY_NAME_SPACE
#endif


#endif //BASEDEFINITIONS_H
