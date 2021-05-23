#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
}; 

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 specular;
    vec3 color;
};


struct Light {
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 Position;
    vec3 Color;
};

uniform Material material;
uniform DirLight dirLight;
uniform Light lights[4];
uniform vec3 viewPos;

void main()
{   

    //направленный свет
    vec3 normal = normalize(fs_in.Normal);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);

    vec3 lightDir = normalize(-dirLight.direction);

    // Диффузная составляющая
    float diff = max(dot(normal, lightDir), 0.0);

    // Отраженная составляющая
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    //
    vec3 ambient = dirLight.ambient * vec3(texture(material.diffuse, fs_in.TexCoords)).rgb;
    vec3 diffuse = dirLight.color * diff * vec3(texture(material.diffuse, fs_in.TexCoords)).rgb;
    vec3 specular = dirLight.specular * spec * vec3(texture(material.specular, fs_in.TexCoords)).rgb;

    vec3 result = (ambient + diffuse + specular) * 0.3;

    // Точечные источники
    for(int i = 0; i < 4; i++)
    {
        vec3 lightDir = normalize(lights[i].Position - fs_in.FragPos);

        // Диффузная составляющая
        float diff = max(dot(normal, lightDir), 0.0);

        // Отраженная составляющая
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
        vec3 specular = lights[i].Color * spec * 0.5;

        // Затухание
        float distance = length(fs_in.FragPos - lights[i].Position);
        float attenuation = 1.0 / (lights[i].constant + lights[i].linear * distance + lights[i].quadratic * (distance * distance));    

        vec3 ambient = lights[i].ambient * vec3(texture(material.diffuse, fs_in.TexCoords)).rgb;
        vec3 diffuse = lights[i].Color * diff * vec3(texture(material.diffuse, fs_in.TexCoords)).rgb;
        
        ambient *= attenuation;
        diffuse *= attenuation;
        specular *= attenuation;
        result = result + (ambient + diffuse + specular) * 0.2;
     }
     //
     FragColor = vec4(result, 1.0);
}