struct VSInput {
	float3 inPosition;
	float2 inUv;
};

struct VSOutput {
	float4 pos : SV_Position;
	float2 uv;
};


struct PushConstants {
	float4x4 mvp;
}

[shader("vertex")]
VSOutput vertMain(VSInput input, uniform PushConstants push) {
	VSOutput output;

	output.pos = float4(input.inPosition, 1.0);
	output.pos = mul(push.mvp, output.pos);

	output.uv = input.inUv;

	return output;
}

Sampler2D texture;
[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target {
	return texture.Sample(vertIn.uv);
}
