#include "../Headers/Randomness_H.hlsl"

//depth and color
Texture2D gDepthTexture         : register(t0);
Texture2D gColorSpecIntTexture  : register(t1);
Texture2D gNormalTexture        : register(t2);
Texture2D gSpecPowTexture       : register(t3);

//buffer for light data
cbuffer PointLightBuffer : register(b3)
{
    float3  lightPos        : packoffset(c0);
    float   radius          : packoffset(c0.w);
    float3  diffuseColor    : packoffset(c1);
    float   intensity       : packoffset(c1.w);
}

cbuffer invProj : register(b4)
{
    matrix  InvProj;
    float   nearPlane;
    float   farPlane;
};

//specular power range
static const float2 cSpecPowerRange = { 0.1, 250.0 };

/////////////////////////////////////////////////////////////////////
// STRUCTS
//input 
struct DS_OUTPUT
{
    float4 Position : SV_POSITION;
    float4 cpPos    : TEXCOORD0;
};

//data from the buffers
struct SURFACE_DATA
{
    float   depth;
    float4  Color;
    float3  Normal;
    float   SpecInt;
    float   SpecPow;
    float   Emissive;
};

//material data
struct Material
{
    float3 normal;
    float4 diffuseColor;
    float specIntensity;
    float specPow;
    float emissive;
};

/////////////////////////////////////////////////////////////////////
// FUNCTIONS
//calculating view position
float3 CalcWorldPos( float2 csPos, float depth )
{
    float4 position;
    position.xy = csPos.xy;
    position.z = depth;
    position.w = 1.0;
    position = mul( position, InvProj );
    position.xyz /= position.w;
    return position.xyz;
}

//unpack the g buffer from our textures
SURFACE_DATA UnpackGBuffer(int2 location)
{
    SURFACE_DATA Out;

    // Cast to 3 component for the load function
    int3 location3 = int3(location, 0);

    // Get the depth value and convert it to linear depth
    float depth = gDepthTexture.Load(location3).x;
    Out.depth = depth;

    // Get the base color and specular intensity
    float4 baseColor = gColorSpecIntTexture.Load(location3);
    Out.Color = baseColor;
    Out.SpecInt;

    // Sample the normal, convert it to the full range and noramalize it
    float4 normalValue = gNormalTexture.Load(location3);
    Out.Normal = normalValue.xyz;
    Out.Normal = normalize(Out.Normal * 2.0 - 1.0);

    //grab emissive value
    Out.Emissive = normalValue.w;

    // Scale the specular power back to the original range
    float4 SpecPowerNorm = gSpecPowTexture.Load(location3);
    Out.SpecPow = cSpecPowerRange.x + SpecPowerNorm.x * cSpecPowerRange.y;
    Out.SpecInt = SpecPowerNorm.w;

    return Out;
}


float3 CalcPoint( float3 position, Material material )
{
    //////////////////////////////////////////////////////////
    //// NEW STUFF

    float3 toLight = lightPos.xyz - position;
    float3 toEye = -position;
    float DistToLight = length(toLight);
    toLight /= DistToLight;

    // Phong diffuse
    float NDotL = saturate(dot(toLight, material.normal));
    float3 finalColor = diffuseColor.rgb * intensity;

    // Blinn specular
    toEye = normalize(toEye);
    float3 HalfWay = normalize(toEye + toLight);
    float NDotH = saturate(dot(HalfWay, material.normal));
    float specularValue = pow(NDotH, material.specPow) * material.specIntensity;

    // Attenuation
    float Attn = saturate(1.0f - (DistToLight / radius));

    float3 finalValue = ((finalColor * material.diffuseColor) + specularValue);

    return Attn * NDotL * finalValue;
}

/////////////////////////////////////////////////////////////////////
// MAIN
float4 main( DS_OUTPUT In ) : SV_TARGET
{
    // Unpack the GBuffer
    SURFACE_DATA gbd = UnpackGBuffer(In.Position.xy);

    // Convert the data into the material structure
    Material mat;
    mat.normal = gbd.Normal;
    mat.diffuseColor = gbd.Color;
    mat.specPow = gbd.SpecPow; 
    mat.specIntensity = gbd.SpecInt;
    mat.emissive = gbd.Emissive;

    //calculate world position
    In.cpPos.xy /= In.cpPos.w;
    float3 pos = CalcWorldPos(In.cpPos.xy, gbd.depth);

    //get the final color
    float4 finalColor;
    finalColor.xyz = CalcPoint(pos, mat);
    finalColor.w = 1.0; 

    return float4(finalColor.xyz, 1.0f);
}