$input a_position, a_normal
$output v_normal

#include <bgfx_shader.sh>

void main()
{
    vec4 worldPos = u_model[0] * vec4(a_position, 1.0);
    vec4 viewPos  = u_view * worldPos;

    gl_Position = u_proj * viewPos;

    vec3 n = (u_view * u_model[0] * vec4(a_normal, 0.0)).xyz;
    v_normal = normalize(n);
}