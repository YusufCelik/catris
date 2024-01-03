#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

// texture sampler
uniform sampler2D texture1;
uniform float alpha; // Uniform to control the alpha value

void main()
{
    vec4 texColor = texture(texture1, TexCoord);
    FragColor = vec4(texColor.rgb, alpha); // Apply the alpha value
}