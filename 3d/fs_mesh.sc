$input v_normal

#include <bgfx_shader.sh>

void main()
{
    vec3 lightDir = normalize(vec3(0.4, 0.6, -0.7));

    float NdotL = max(dot(normalize(v_normal), lightDir), 0.0);

    vec3 baseColor = vec3(0.8, 0.8, 0.85);
    vec3 color = baseColor * (0.2 + 0.8 * NdotL);

    gl_FragColor = vec4(color, 1.0);
}