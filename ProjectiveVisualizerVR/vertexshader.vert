#version 330 core
layout(location = 0) in vec4 vertexPosition_modelspace;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec4 vertexNormal;

uniform mat4 projection;

out vec3 fragmentColor;

void main(void)
{
    gl_Position = projection*vertexPosition_modelspace;

    fragmentColor = vec3(0.1,0.5,1.0) * abs(dot(vertexNormal, vec4(0,0.7,0.7,0))) + 0.003*dot(vertexPosition_modelspace,vertexPosition_modelspace);
}
