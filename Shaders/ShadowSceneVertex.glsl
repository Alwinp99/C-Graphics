#version 330 core
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

in vec3 position;
in vec4 colour;
in vec3 normal; //New Attribute!
in vec4 tangent ; 
in vec2 texCoord;

out Vertex {
	vec4 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 tangent ; // New ! Note , Vec3 !
	vec3 binormal ; // New !
	vec3 worldPos;
} OUT;

void main(void) {
	OUT.colour = colour;
	OUT.texCoord = texCoord;

	mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));

	OUT.normal = normalize(normalMatrix * normalize(normal));
	OUT.tangent = normalize ( normalMatrix * normalize (tangent.xyz));

	OUT.binormal = cross(OUT.normal,OUT.tangent) * tangent.w;


	vec4 worldPos = (modelMatrix * vec4(position ,1));

	OUT.worldPos = worldPos.xyz;

	gl_Position = (projMatrix * viewMatrix) * worldPos;
 }