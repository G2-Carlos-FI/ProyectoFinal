#version 330 core
out vec4 FragColor;
  
in vec3 Color;
in vec2 TexCoords; // <-- Corregido de TexCoord

uniform sampler2D ourTexture;
    
void main()
{
     vec4 texColor = texture(ourTexture, TexCoords); // <-- Corregido de TexCoord
     if(texColor.a < 0.1)
        discard;
        
     FragColor = texColor; // <-- Usar FragColor
}