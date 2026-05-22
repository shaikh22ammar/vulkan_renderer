struct VSInput {
	float2 inPosition;
	float3 inColor;
};

struct VSOutput {
	float4 pos : SV_Position;
	float3 color;
};

struct MVPmatrices {
	float4x4 model;
	float4x4 view;
	float4x4 proj;
};

ConstantBuffer<MVPmatrices> mvp;

[shader("vertex")]
VSOutput vertMain(VSInput input) {
	VSOutput output;
	output.pos = float4(input.inPosition, 0.0, 1.0);
	output.pos = mul(mvp.proj, mul(mvp.view, (mul(mvp.model, output.pos))));
	output.color = input.inColor;
	return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target {
	return float4(vertIn.color, 1.0);
}
