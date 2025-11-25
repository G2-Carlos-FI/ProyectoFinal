#version 330 core
out vec4 FragColor;

// --- ESTRUCTURAS DE LUZ ---
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// --- ESTRUCTURA DE MATERIAL ---
struct Material {
    vec3 ambient;
    vec3 diffuse; // (Ahora solo se usará para color sólido)
    vec3 specular;
    float shininess;
}; 

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords; // (¡Ahora sí lo usaremos!)

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[2];
uniform Material material;

// --- ¡¡NUEVAS LÍNEAS!! ---
uniform sampler2D texture_diffuse1; // Sampler para el pasto, agua Y modelos
uniform bool useTexture;            // El "interruptor"

// Prototipos de funciones
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffuseColor);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor);

void main()
{
    // Propiedades
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // --- ¡¡LÓGICA MODIFICADA!! ---
    // Determinamos el color difuso base (de textura o de material)
    vec3 diffuseColor;
    if(useTexture)
    {
        // 1. Es un objeto con textura (pasto, agua, Pokémon)
        vec4 texColor = texture(texture_diffuse1, TexCoords);
        if(texColor.a < 0.1) // Mantenemos el descarte por transparencia
            discard;
        diffuseColor = texColor.rgb;
    }
    else
    {
        // 2. Es un objeto de color sólido (casas, árboles)
        diffuseColor = material.diffuse;
    }
    // --- FIN DE LÓGICA MODIFICADA ---

    
    // Fase 1: Luz Direccional (Sol o Luna)
    vec3 result = CalcDirLight(dirLight, norm, viewDir, diffuseColor);
    
    // Fase 2: Luces de Punto (Postes)
    result += CalcPointLight(pointLights[0], norm, FragPos, viewDir, diffuseColor);
    result += CalcPointLight(pointLights[1], norm, FragPos, viewDir, diffuseColor);
    
    FragColor = vec4(result, 1.0);
}

// --- Función para la Luz Direccional (MODIFICADA) ---
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffuseColor)
{
    vec3 lightDir = normalize(-light.direction);
    // Luz Ambiente
    vec3 ambient = light.ambient * diffuseColor; // <-- Usa el color difuso (de textura o material)
    // Luz Difusa
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * diffuseColor; // <-- Usa el color difuso
    // Luz Especular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * material.specular;
		
    return (ambient + diffuse + specular);
}

// --- Función para las Luces de Punto (MODIFICADA) ---
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // Luz Ambiente
    vec3 ambient = light.ambient * diffuseColor; // <-- Usa el color difuso
    // Luz Difusa
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * diffuseColor; // <-- Usa el color difuso
    // Luz Especular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * material.specular;
    // Atenuación
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
		
    return (ambient + diffuse + specular);
}