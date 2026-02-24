$input v_normal, v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);

void main()
{
    vec4 color = texture2D(s_texColor, v_texcoord0);

    // Simple diffuse lighting using world normal
    vec3 lightDir = normalize(vec3(1.0, 2.0, 1.0));
    float diff = max(dot(normalize(v_normal), lightDir), 0.2);

    gl_FragColor = vec4(color.rgb * diff, color.a);
}