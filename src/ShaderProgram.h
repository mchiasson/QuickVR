#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include <QOpenGLExtraFunctions>
#include <QMatrix4x4>

#include "DirectionalLight.h"
#include "PointLight.h"
#include "Shader.h"
#include "SpotLight.h"

#define MAX_POINT_LIGHTS 4
#define MAX_SPOT_LIGHTS 4
#define MAX_BONES 100

class ShaderProgram : public Node
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<Shader> shaders READ shaders)
    Q_PROPERTY(GLuint  handle                   READ handle      NOTIFY handleChanged)
    Q_PROPERTY(bool    linkStatus               READ linkStatus  NOTIFY linkStatusChanged)
    Q_PROPERTY(QString linkInfoLog              READ linkInfoLog NOTIFY linkInfoLogChanged)

public:

    explicit ShaderProgram(Node *parent = nullptr) : Node(parent) { }

    QQmlListProperty<Shader> shaders();
    void appendShader(Shader*);
    int shaderCount() const;
    Shader *shader(int) const;
    void clearShaders();
    void replaceShader(int, Shader*);
    void removeLastShader();

    GLuint handle() const { return m_handle; }
    bool linkStatus() const { return m_linkStatus == GL_TRUE; }
    const QString &linkInfoLog() const { return m_linkInfoLog; }

    void setHandle(GLuint newHandle);
    void setLinkStatus(GLint newLinkStatus);
    void setLinkInfoLog(const QString &newLinkInfoLog);

    void bind();
    GLint getUniformLocation(const char* pUniformName);
    GLint getProgramParam(GLint param);

    void setMVP(const QMatrix4x4& mvp);
    void setModel(const QMatrix4x4 &model);
    void setColorTextureUnit(uint textureUnit);
    void setDirectionalLight(const DirectionalLight& light);
    void setPointLights(uint NumLights, const PointLight* lights);
    void setSpotLights(uint NumLights, const SpotLight* lights);
    void setEyeWorldPos(const QVector3D& eyeWorldPos);
    void setMatSpecularIntensity(float intensity);
    void setMatSpecularPower(float power);
    void setBoneTransform(uint index, const QMatrix4x4& transform);

signals:

    void handleChanged(GLuint handle);
    void linkStatusChanged(bool newLinkStatus);
    void linkInfoLogChanged(const QString &newLinkInfoLog);

protected slots:


protected:
    virtual void onInit() override;
    virtual void onShutdown() override;
    virtual void onUpdate() override;

    static void appendShader(QQmlListProperty<Shader>*, Shader*);
    static int shaderCount(QQmlListProperty<Shader>*);
    static Shader* shader(QQmlListProperty<Shader>*, int);
    static void clearShaders(QQmlListProperty<Shader>*);
    static void replaceShader(QQmlListProperty<Shader>*, int, Shader*);
    static void removeLastShader(QQmlListProperty<Shader>*);

    QVector<Shader*> m_shaders;
    GLuint m_handle = 0;
    GLint m_linkStatus = GL_FALSE;
    QString m_linkInfoLog;

    GLuint m_MVPLocation;
    GLuint m_modelLocation;
    GLuint m_colorTextureLocation;
    GLuint m_eyeWorldPosLocation;
    GLuint m_matSpecularIntensityLocation;
    GLuint m_matSpecularPowerLocation;
    GLuint m_numPointLightsLocation;
    GLuint m_numSpotLightsLocation;

    struct {
        GLuint Color;
        GLuint AmbientIntensity;
        GLuint DiffuseIntensity;
        GLuint Direction;
    } m_dirLightLocation;

    struct {
        GLuint Color;
        GLuint AmbientIntensity;
        GLuint DiffuseIntensity;
        GLuint Position;
        struct {
            GLuint Constant;
            GLuint Linear;
            GLuint Exp;
        } Atten;
    } m_pointLightsLocation[MAX_POINT_LIGHTS];

    struct {
        GLuint Color;
        GLuint AmbientIntensity;
        GLuint DiffuseIntensity;
        GLuint Position;
        GLuint Direction;
        GLuint Cutoff;
        struct {
            GLuint Constant;
            GLuint Linear;
            GLuint Exp;
        } Atten;
    } m_spotLightsLocation[MAX_SPOT_LIGHTS];

    GLuint m_boneLocation[MAX_BONES];
};

#endif /* SHADERPROGRAM_H */

