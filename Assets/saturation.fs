// saturation.fs
#ifdef GL_ES
precision mediump float;
#endif

// The texture sampler (set automatically by Raylib)
uniform sampler2D texture0;
// Saturation factor (1.0 = no change, >1.0 increases saturation)
uniform float saturation;

varying vec2 fragTexCoord;

vec3 AdjustSaturation(vec3 color, float saturation)
{
    // Compute luminance (grayscale) using standard weights
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    // Interpolate between gray and the original color
    return mix(vec3(gray), color, saturation);
}

void main(void)
{
    vec4 texColor = texture2D(texture0, fragTexCoord);
    texColor.rgb = AdjustSaturation(texColor.rgb, saturation);
    gl_FragColor = texColor;
}
