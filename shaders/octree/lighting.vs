#version 430 core

uniform mat4 mvp;
uniform mat4 normal_mat;

layout(location=0) in vec4 in_vertex;
layout(location=2) in vec3 in_normal;

out vec3 vs_normal;

void main()
{
	vs_normal = (normal_mat * vec4(in_normal,0)).xyz;
  gl_Position = mvp * in_vertex;
}
