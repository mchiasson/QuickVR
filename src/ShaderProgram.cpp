#include "ShaderProgram.h"

#include <QDebug>
#include <QFile>
#include <QtMath>

#include "Device.h"

QQmlListProperty<Shader> ShaderProgram::shaders()
{
    return {this, this,
            [](QQmlListProperty<Shader> *prop, Shader *newShader)
            {
                ShaderProgram *program = qobject_cast<ShaderProgram*>(prop->object);
                if (program) {
                    program->appendShader(newShader);
                }
            },
            [](QQmlListProperty<Shader> *prop)->int
            {
                ShaderProgram *program = qobject_cast<ShaderProgram*>(prop->object);
                if (program) {
                    return program->shaderCount();
                }
                return 0;
            },
            [](QQmlListProperty<Shader> *prop, int index)->Shader*
            {
                ShaderProgram *program = qobject_cast<ShaderProgram*>(prop->object);
                if (program) {
                    return program->shader(index);
                }
                return nullptr;
            },
            [](QQmlListProperty<Shader> *prop)
            {
                ShaderProgram *program = qobject_cast<ShaderProgram*>(prop->object);
                if (program) {
                    program->clearShaders();
                }
            },
            [](QQmlListProperty<Shader> *prop, int index, Shader* newShader)
            {
                ShaderProgram *program = qobject_cast<ShaderProgram*>(prop->object);
                if (program) {
                    program->replaceShader(index, newShader);
                }
            },
            [](QQmlListProperty<Shader> *prop)
            {
                ShaderProgram *program = qobject_cast<ShaderProgram*>(prop->object);
                if (program) {
                    program->removeLastShader();
                }
            }
    };
}

void ShaderProgram::appendShader(Shader* p) {
    m_shaders.append(p);
    setLinkStatus(GL_FALSE);
}

int ShaderProgram::shaderCount() const
{
    return m_shaders.count();
}

Shader *ShaderProgram::shader(int index) const
{
    return m_shaders.at(index);
}

void ShaderProgram::clearShaders() {
    m_shaders.clear();
    setLinkStatus(GL_FALSE);
}

void ShaderProgram::replaceShader(int index, Shader *p)
{
    m_shaders[index] = p;
    setLinkStatus(GL_FALSE);
}

void ShaderProgram::removeLastShader()
{
    m_shaders.removeLast();
    setLinkStatus(GL_FALSE);
}

void ShaderProgram::setHandle(GLuint newHandle)
{
    if (m_handle != newHandle)
    {
        m_handle = newHandle;
        emit handleChanged(newHandle);
    }
}

void ShaderProgram::setLinkStatus(GLint newLinkStatus)
{
    if (m_linkStatus != newLinkStatus)
    {
        m_linkStatus = newLinkStatus;
        emit linkStatusChanged(newLinkStatus);
    }
}

void ShaderProgram::setLinkInfoLog(const QString &newLinkInfoLog)
{
    if (m_linkInfoLog != newLinkInfoLog)
    {
        m_linkInfoLog = newLinkInfoLog;
        emit linkInfoLogChanged(newLinkInfoLog);
    }
}

void ShaderProgram::bind()
{
    device()->glUseProgram(handle());
}

GLint ShaderProgram::getUniformLocation(const char* pUniformName)
{
    GLuint location = device()->glGetUniformLocation(handle(), pUniformName);

    if (location == -1)
    {
        qWarning() << "Warning! Unable to get the location of uniform" << pUniformName;
    }

    return location;
}

GLint ShaderProgram::getProgramParam(GLint param)
{
    GLint ret;
    device()->glGetProgramiv(handle(), param, &ret);
    return ret;
}


void ShaderProgram::setMVP(const glm::mat4& mvp)
{
    device()->glUniformMatrix4fv(m_MVPLocation, 1, GL_TRUE, &mvp[0][0]);
}

void ShaderProgram::setModel(const glm::mat4 &model)
{
    device()->glUniformMatrix4fv(m_modelLocation, 1, GL_TRUE, &model[0][0]);
}

void ShaderProgram::setColorTextureUnit(uint textureUnit)
{
    device()->glUniform1i(m_colorTextureLocation, textureUnit);
}

void ShaderProgram::setDirectionalLight(const DirectionalLight& light)
{
    device()->glUniform3f(m_dirLightLocation.Color, light.color().redF(), light.color().greenF(), light.color().blueF());
    device()->glUniform1f(m_dirLightLocation.AmbientIntensity, light.ambientIntensity());
    glm::vec3 direction = glm::normalize(glm::vec3(light.direction().x(), light.direction().y(), light.direction().z()));
    device()->glUniform3f(m_dirLightLocation.Direction, direction.x, direction.y, direction.z);
    device()->glUniform1f(m_dirLightLocation.DiffuseIntensity, light.diffuseIntensity());
}

void ShaderProgram::setPointLights(uint NumLights, const PointLight* pLights)
{
    device()->glUniform1i(m_numPointLightsLocation, NumLights);

    for (unsigned int i = 0 ; i < NumLights ; i++) {
        device()->glUniform3f(m_pointLightsLocation[i].Color, pLights[i].color().redF(), pLights[i].color().greenF(), pLights[i].color().blueF());
        device()->glUniform1f(m_pointLightsLocation[i].AmbientIntensity, pLights[i].ambientIntensity());
        device()->glUniform1f(m_pointLightsLocation[i].DiffuseIntensity, pLights[i].diffuseIntensity());
        device()->glUniform3f(m_pointLightsLocation[i].Position, pLights[i].x(), pLights[i].y(), pLights[i].z());
        device()->glUniform1f(m_pointLightsLocation[i].Atten.Constant, pLights[i].attenuationConstant());
        device()->glUniform1f(m_pointLightsLocation[i].Atten.Linear, pLights[i].attenuationLinear());
        device()->glUniform1f(m_pointLightsLocation[i].Atten.Exp, pLights[i].attenuationExp());
    }
}

void ShaderProgram::setSpotLights(uint NumLights, const SpotLight* pLights)
{
    device()->glUniform1i(m_numSpotLightsLocation, NumLights);

    for (unsigned int i = 0 ; i < NumLights ; i++) {
        device()->glUniform3f(m_spotLightsLocation[i].Color, pLights[i].color().redF(), pLights[i].color().greenF(), pLights[i].color().blueF());
        device()->glUniform1f(m_spotLightsLocation[i].AmbientIntensity, pLights[i].ambientIntensity());
        device()->glUniform1f(m_spotLightsLocation[i].DiffuseIntensity, pLights[i].diffuseIntensity());
        device()->glUniform3f(m_spotLightsLocation[i].Position, pLights[i].x(), pLights[i].y(), pLights[i].z());
        glm::vec3 direction = glm::normalize(glm::vec3(pLights[i].direction().x(), pLights[i].direction().y(), pLights[i].direction().z()));
        device()->glUniform3f(m_spotLightsLocation[i].Direction, direction.x, direction.y, direction.z);
        device()->glUniform1f(m_spotLightsLocation[i].Cutoff, cosf(qDegreesToRadians(pLights[i].cutoff())));
        device()->glUniform1f(m_spotLightsLocation[i].Atten.Constant, pLights[i].attenuationConstant());
        device()->glUniform1f(m_spotLightsLocation[i].Atten.Linear,   pLights[i].attenuationLinear());
        device()->glUniform1f(m_spotLightsLocation[i].Atten.Exp,      pLights[i].attenuationExp());
    }
}

void ShaderProgram::setEyeWorldPos(const glm::vec3& eyeWorldPos)
{
    device()->glUniform3f(m_eyeWorldPosLocation, eyeWorldPos.x, eyeWorldPos.y, eyeWorldPos.z);
}

void ShaderProgram::setMatSpecularIntensity(float intensity)
{
    device()->glUniform1f(m_matSpecularIntensityLocation, intensity);
}

void ShaderProgram::setMatSpecularPower(float power)
{
    device()->glUniform1f(m_matSpecularPowerLocation, power);
}

void ShaderProgram::setBoneTransform(uint index, const glm::mat4& transform)
{
    device()->glUniformMatrix4fv(m_boneLocation[index].Bone, 1, GL_TRUE, &transform[0][0]);
}

void ShaderProgram::onInit()
{
    setLinkStatus(GL_FALSE);
}

void ShaderProgram::onShutdown()
{
    device()->glDeleteProgram(handle());
    setHandle(0);
    setLinkStatus(GL_FALSE);
}

void ShaderProgram::onUpdate()
{
    if (linkStatus() == GL_TRUE) return; // already linked.

    // make sure the shaders are all present and compiled.

    if (shaderCount() < 2) return;

    for (int i = 0; i < shaderCount(); ++i)
    {
        if (shader(i)->handle() == 0 || !shader(i)->compileStatus()) return;
    }

    if (handle())
    {
        device()->glDeleteProgram(handle());
    }

    setHandle(device()->glCreateProgram());

    for (int i = 0; i < shaderCount(); ++i)
    {
        device()->glAttachShader(handle(), shader(i)->handle());
    }

    device()->glLinkProgram(handle());

    GLint linkStatus = 0;
    device()->glGetProgramiv(handle(), GL_LINK_STATUS, &linkStatus);
    setLinkStatus(linkStatus);

    if (linkStatus == GL_TRUE)
    {
        device()->glValidateProgram(handle());
        device()->glGetProgramiv(handle(), GL_VALIDATE_STATUS, &linkStatus);
        setLinkStatus(linkStatus);

        if (linkStatus == GL_TRUE)
        {
            char Name[128];

            setLinkInfoLog(QString());

            m_MVPLocation                       = getUniformLocation("uMVP");
            m_modelLocation                     = getUniformLocation("uModel");
            m_colorTextureLocation              = getUniformLocation("uColorMap");
            m_eyeWorldPosLocation               = getUniformLocation("uEyeWorldPos");
            m_dirLightLocation.Color            = getUniformLocation("uDirectionalLight.Base.Color");
            m_dirLightLocation.AmbientIntensity = getUniformLocation("uDirectionalLight.Base.AmbientIntensity");
            m_dirLightLocation.Direction        = getUniformLocation("uDirectionalLight.Direction");
            m_dirLightLocation.DiffuseIntensity = getUniformLocation("uDirectionalLight.Base.DiffuseIntensity");
            m_matSpecularIntensityLocation      = getUniformLocation("uMatSpecularIntensity");
            m_matSpecularPowerLocation          = getUniformLocation("uSpecularPower");
            m_numPointLightsLocation            = getUniformLocation("uNumPointLights");
            m_numSpotLightsLocation             = getUniformLocation("uNumSpotLights");

            for (unsigned int i = 0 ; i < ARRAY_SIZE_IN_ELEMENTS(m_pointLightsLocation) ; i++) {
                memset(Name, 0, sizeof(Name));
                snprintf(Name, sizeof(Name), "uPointLights[%d].Base.Color", i);
                m_pointLightsLocation[i].Color = getUniformLocation(Name);

                snprintf(Name, sizeof(Name), "uPointLights[%d].Base.AmbientIntensity", i);
                m_pointLightsLocation[i].AmbientIntensity = getUniformLocation(Name);

                snprintf(Name, sizeof(Name), "uPointLights[%d].Position", i);
                m_pointLightsLocation[i].Position = getUniformLocation(Name);

                snprintf(Name, sizeof(Name), "uPointLights[%d].Base.DiffuseIntensity", i);
                m_pointLightsLocation[i].DiffuseIntensity = getUniformLocation(Name);

                snprintf(Name, sizeof(Name), "uPointLights[%d].Atten.Constant", i);
                m_pointLightsLocation[i].Atten.Constant = getUniformLocation(Name);

                snprintf(Name, sizeof(Name), "uPointLights[%d].Atten.Linear", i);
                m_pointLightsLocation[i].Atten.Linear = getUniformLocation(Name);

                snprintf(Name, sizeof(Name), "uPointLights[%d].Atten.Exp", i);
                m_pointLightsLocation[i].Atten.Exp = getUniformLocation(Name);
            }

            for (unsigned int i = 0 ; i < ARRAY_SIZE_IN_ELEMENTS(m_spotLightsLocation) ; i++) {
                memset(Name, 0, sizeof(Name));
                snprintf(Name, sizeof(Name), "uSpotLights[%d].Base.Base.Color", i);
                m_spotLightsLocation[i].Color = getUniformLocation(Name);

                snprintf(Name, sizeof(Name), "uSpotLights[%d].Base.Base.AmbientIntensity", i);
                m_spotLightsLocation[i].AmbientIntensity = getUniformLocation(Name);

                snprintf(Name, sizeof(Name), "uSpotLights[%d].Base.Position", i);
                m_spotLightsLocation[i].Position = getUniformLocation(Name);

                snprintf(Name, sizeof(Name), "uSpotLights[%d].Direction", i);
                m_spotLightsLocation[i].Direction = getUniformLocation(Name);

                snprintf(Name, sizeof(Name), "uSpotLights[%d].Cutoff", i);
                m_spotLightsLocation[i].Cutoff = getUniformLocation(Name);

                snprintf(Name, sizeof(Name), "uSpotLights[%d].Base.Base.DiffuseIntensity", i);
                m_spotLightsLocation[i].DiffuseIntensity = getUniformLocation(Name);

                snprintf(Name, sizeof(Name), "uSpotLights[%d].Base.Atten.Constant", i);
                m_spotLightsLocation[i].Atten.Constant = getUniformLocation(Name);

                snprintf(Name, sizeof(Name), "uSpotLights[%d].Base.Atten.Linear", i);
                m_spotLightsLocation[i].Atten.Linear = getUniformLocation(Name);

                snprintf(Name, sizeof(Name), "uSpotLights[%d].Base.Atten.Exp", i);
                m_spotLightsLocation[i].Atten.Exp = getUniformLocation(Name);
            }

            for (unsigned int i = 0 ; i < ARRAY_SIZE_IN_ELEMENTS(m_boneLocation) ; i++) {
                memset(Name, 0, sizeof(Name));
                snprintf(Name, sizeof(Name), "uBones[%d]", i);
                m_boneLocation[i].Bone = getUniformLocation(Name);
            }
        }
        else
        {
            GLint infoLogLength = 0;
            device()->glGetProgramiv(handle(), GL_INFO_LOG_LENGTH, &infoLogLength);

            QByteArray infoLog;
            infoLog.resize(infoLogLength);
            device()->glGetProgramInfoLog(handle(), infoLogLength, NULL, infoLog.data());
            qCritical() << "Could not validate program :" << qPrintable(infoLog);
            setLinkInfoLog(infoLog);
        }
    }
    else
    {
        GLint infoLogLength = 0;
        device()->glGetProgramiv(handle(), GL_INFO_LOG_LENGTH, &infoLogLength);

        QByteArray infoLog;
        infoLog.resize(infoLogLength);
        device()->glGetProgramInfoLog(handle(), infoLogLength, NULL, infoLog.data());
        qCritical() << "Could not link program :" << qPrintable(infoLog);
        setLinkInfoLog(infoLog);
    }

}

void ShaderProgram::appendShader(QQmlListProperty<Shader> *prop, Shader *newShader)
{
    ShaderProgram *program = qobject_cast<ShaderProgram*>(prop->object);
    if (program) {
        program->appendShader(newShader);
    }
}

int ShaderProgram::shaderCount(QQmlListProperty<Shader> *prop)
{
    ShaderProgram *program = qobject_cast<ShaderProgram*>(prop->object);
    if (program) {
        return program->shaderCount();
    }
    return 0;
}

Shader* ShaderProgram::shader(QQmlListProperty<Shader> *prop, int index)
{
    ShaderProgram *program = qobject_cast<ShaderProgram*>(prop->object);
    if (program) {
        return program->shader(index);
    }
    return nullptr;
}

void ShaderProgram::clearShaders(QQmlListProperty<Shader> *prop)
{
    ShaderProgram *program = qobject_cast<ShaderProgram*>(prop->object);
    if (program) {
        program->clearShaders();
    }
}

void ShaderProgram::replaceShader(QQmlListProperty<Shader> *prop, int index, Shader* newShader)
{
    ShaderProgram *program = qobject_cast<ShaderProgram*>(prop->object);
    if (program) {
        program->replaceShader(index, newShader);
    }
}

void ShaderProgram::removeLastShader(QQmlListProperty<Shader> *prop)
{
    ShaderProgram *program = qobject_cast<ShaderProgram*>(prop->object);
    if (program) {
        program->removeLastShader();
    }
}
