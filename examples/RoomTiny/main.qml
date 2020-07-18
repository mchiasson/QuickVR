import QtQuick        2.15
import QtQuick.Window 2.15
import QuickVR 1.0

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("RoomTiny")
    color: "skyblue"

    Device {
        anchors.fill: parent

        headset: Headset {
            id: headset
            x:  0
            y:  0
            z: -5
            //        transform: Rotation {
            //            axis { x: 0; y: 1; z: 0 }
            //            angle: 180
            //        }
            rotation: 180;

            focus: true
            Keys.onPressed: handleKeyEvent(event, true);
            Keys.onReleased: handleKeyEvent(event, false);

            function handleKeyEvent(event, pressed)
            {
                if (!event.isAutoRepeat) {
                    if (event.key === Qt.Key_Left)
                    {
                        angularVelocity.y += pressed ? 0.75 : -0.75;
                        event.accepted = true;
                    }
                    else if (event.key === Qt.Key_Right)
                    {
                        angularVelocity.y += pressed ? -0.75 : 0.75;
                        event.accepted = true;
                    }
                    else if (event.key === Qt.Key_W || event.key === Qt.Key_Up)
                    {
                        linearVelocity.z += pressed ? -0.05 : 0.05;
                        event.accepted = true;
                    }
                    else if (event.key === Qt.Key_S || event.key === Qt.Key_Down)
                    {
                        linearVelocity.z += pressed ? 0.05 : -0.05;
                        event.accepted = true;
                    }
                    else if (event.key === Qt.Key_A)
                    {
                        linearVelocity.x += pressed ? -0.05 : 0.05;
                        event.accepted = true;
                    }
                    else if (event.key === Qt.Key_D)
                    {
                        linearVelocity.x += pressed ? 0.05 : -0.05;
                        event.accepted = true;
                    }
                }
            }
        }

        Shader {
            id: skinningVertShader
            type: Shader.Vertex
            source: "
                #version 330

                layout (location = 0) in vec3 Position;
                layout (location = 1) in vec2 TexCoord;
                layout (location = 2) in vec3 Normal;
                layout (location = 3) in ivec4 BoneIDs;
                layout (location = 4) in vec4 Weights;

                out vec2 TexCoord0;
                out vec3 Normal0;
                out vec3 WorldPos0;

                const int MAX_BONES = 100;

                uniform mat4 uMVP;
                uniform mat4 uModel;
                uniform mat4 uBones[MAX_BONES];

                void main()
                {
                    mat4 BoneTransform = uBones[BoneIDs[0]] * Weights[0];
                    BoneTransform     += uBones[BoneIDs[1]] * Weights[1];
                    BoneTransform     += uBones[BoneIDs[2]] * Weights[2];
                    BoneTransform     += uBones[BoneIDs[3]] * Weights[3];

                    vec4 PosL    = BoneTransform * vec4(Position, 1.0);
                    gl_Position  = uMVP * PosL;
                    TexCoord0    = TexCoord;
                    vec4 NormalL = BoneTransform * vec4(Normal, 0.0);
                    Normal0      = (uModel * NormalL).xyz;
                    WorldPos0    = (uModel * PosL).xyz;
                }
            "
        }

        Shader {
            id: skinningFragShader
            type: Shader.Fragment
            source: "
                #version 330

                const int MAX_POINT_LIGHTS = 4;
                const int MAX_SPOT_LIGHTS = 4;

                in vec2 TexCoord0;
                in vec3 Normal0;
                in vec3 WorldPos0;

                struct VSOutput
                {
                    vec2 TexCoord;
                    vec3 Normal;
                    vec3 WorldPos;
                };

                struct BaseLight
                {
                    vec3 Color;
                    float AmbientIntensity;
                    float DiffuseIntensity;
                };

                struct DirectionalLight
                {
                    BaseLight Base;
                    vec3 Direction;
                };

                struct Attenuation
                {
                    float Constant;
                    float Linear;
                    float Exp;
                };

                struct PointLight
                {
                    BaseLight Base;
                    vec3 Position;
                    Attenuation Atten;
                };

                struct SpotLight
                {
                    PointLight Base;
                    vec3 Direction;
                    float Cutoff;
                };

                uniform int uNumPointLights;
                uniform int uNumSpotLights;
                uniform DirectionalLight uDirectionalLight;
                uniform PointLight uPointLights[MAX_POINT_LIGHTS];
                uniform SpotLight uSpotLights[MAX_SPOT_LIGHTS];
                uniform sampler2D uColorMap;
                uniform vec3 uEyeWorldPos;
                uniform float uMatSpecularIntensity;
                uniform float uSpecularPower;


                vec4 CalcLightInternal(BaseLight Light, vec3 LightDirection, VSOutput In)
                {
                    vec4 AmbientColor = vec4(Light.Color * Light.AmbientIntensity, 1.0);
                    float DiffuseFactor = dot(In.Normal, -LightDirection);

                    vec4 DiffuseColor  = vec4(0, 0, 0, 0);
                    vec4 SpecularColor = vec4(0, 0, 0, 0);

                    if (DiffuseFactor > 0.0) {
                        DiffuseColor = vec4(Light.Color * Light.DiffuseIntensity * DiffuseFactor, 1.0);

                        vec3 VertexToEye = normalize(uEyeWorldPos - In.WorldPos);
                        vec3 LightReflect = normalize(reflect(LightDirection, In.Normal));
                        float SpecularFactor = dot(VertexToEye, LightReflect);
                        if (SpecularFactor > 0.0) {
                            SpecularFactor = pow(SpecularFactor, uSpecularPower);
                            SpecularColor = vec4(Light.Color * uMatSpecularIntensity * SpecularFactor, 1.0);
                        }
                    }

                    return (AmbientColor + DiffuseColor + SpecularColor);
                }

                vec4 CalcDirectionalLight(VSOutput In)
                {
                    return CalcLightInternal(uDirectionalLight.Base, uDirectionalLight.Direction, In);
                }

                vec4 CalcPointLight(PointLight l, VSOutput In)
                {
                    vec3 LightDirection = In.WorldPos - l.Position;
                    float Distance = length(LightDirection);
                    LightDirection = normalize(LightDirection);

                    vec4 Color = CalcLightInternal(l.Base, LightDirection, In);
                    float Attenuation =  l.Atten.Constant +
                                         l.Atten.Linear * Distance +
                                         l.Atten.Exp * Distance * Distance;

                    return Color / Attenuation;
                }

                vec4 CalcSpotLight(SpotLight l, VSOutput In)
                {
                    vec3 LightToPixel = normalize(In.WorldPos - l.Base.Position);
                    float SpotFactor = dot(LightToPixel, l.Direction);

                    if (SpotFactor > l.Cutoff) {
                        vec4 Color = CalcPointLight(l.Base, In);
                        return Color * (1.0 - (1.0 - SpotFactor) * 1.0/(1.0 - l.Cutoff));
                    }
                    else {
                        return vec4(0,0,0,0);
                    }
                }

                out vec4 FragColor;

                void main()
                {
                    VSOutput In;
                    In.TexCoord = TexCoord0;
                    In.Normal   = normalize(Normal0);
                    In.WorldPos = WorldPos0;

                    vec4 TotalLight = CalcDirectionalLight(In);

                    for (int i = 0 ; i < uNumPointLights ; i++) {
                        TotalLight += CalcPointLight(uPointLights[i], In);
                    }

                    for (int i = 0 ; i < uNumSpotLights ; i++) {
                        TotalLight += CalcSpotLight(uSpotLights[i], In);
                    }

                    FragColor = texture(uColorMap, In.TexCoord.xy) * TotalLight;
                }
            "
        }

        ShaderProgram {
            id: skinningShaderProgram
            shaders: [skinningVertShader, skinningFragShader]
        }

        SkinnedMesh {
            shaderProgram: skinningShaderProgram
            source: "../../../ogldev/Content/boblampclean.md5mesh"
        }

//        lights: [
//            DirectionalLight {
//                color: "white"
//                ambientIntensity: 0.55
//                diffuseIntensity: 0.9
//                direction: "1, 0, 0"
//            }
//        ]

    }


}
