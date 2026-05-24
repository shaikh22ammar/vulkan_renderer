struct VSInput {
	float2 inPosition;
	float3 inColor;
};

struct VSOutput {
	float4 pos : SV_Position;
	float3 color;
};


struct PushConstants {
	float4x4 mvp;
}

[shader("vertex")]
VSOutput vertMain(VSInput input, uniform PushConstants push) {
	VSOutput output;

	output.pos = float4(input.inPosition, 0.0, 1.0);
	output.pos = mul(push.mvp, output.pos);

	output.color = input.inColor;

	return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target {
	return float4(vertIn.color, 1.0);
}
