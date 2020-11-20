R""(
#version 150

in vec3 Normal;
in vec4 Color;
in vec2 UV1;
in vec2 UV2;

out vec4 outColor;

uniform float alphaCutoff;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform int numTextures;
uniform int texVoreinstellung;

void main() {
  vec4 texColor = (numTextures == 0) ? vec4(1.0, 1.0, 1.0, 1.0) : texture(tex1, UV1);

  if (texColor.a < alphaCutoff) {
    discard;
  }

  if (texVoreinstellung == 3) {
    // Tex 1 Standard, Tex 2 transparent
    vec4 tex2Color = texture(tex2, UV2);
    texColor = mix(texColor, tex2Color, tex2Color.a);
  }

  if (texVoreinstellung == 4 || (texVoreinstellung >= 6 && texVoreinstellung <= 9)) {
    // Halbtransparenz
    if (numTextures == 0) {
      // Nur wenn keine Textur angegeben ist, wird vom Farbwert auch die Alpha-Komponente verwendet.
      // Die wird wiederum nur gebraucht, wenn Alpha-Blending aktiv ist.
      outColor = Color;
    } else {
      outColor = texColor * vec4(Color.rgb, 1.0);
    }
  } else {
    outColor = vec4(texColor.rgb * Color.rgb, 1.0);
  }

  const float ambient = 0.4;
  float kd = ambient + max(0.0, (1 - ambient) * dot(normalize(vec3(0, -1, 1)), Normal));
  outColor *= vec4(kd, kd, kd, 1.0);
}
)""
