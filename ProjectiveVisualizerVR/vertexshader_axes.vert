#version 330 core
layout(location = 0) in vec3 vertexPosition;

uniform mat4x4 projection;
uniform vec4 color;

out vec3 fragmentColor;

void main(void)
{
    gl_Position = projection * vec4(vertexPosition,1);
    fragmentColor = color;
}
