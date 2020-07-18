#include "Shader.h"

#include <QDebug>
#include <QFile>

#include "Device.h"

void Shader::setSource(const QString &newSource)
{
    if (m_source != newSource)
    {
        m_source = newSource;
        emit sourceChanged(newSource);
        setCompileStatus(GL_FALSE);
    }
}

void Shader::setType(Type newType)
{
    if (m_type != newType)
    {
        m_type = newType;
        emit typeChanged(newType);
        setCompileStatus(GL_FALSE);
    }
}

void Shader::setHandle(GLuint newHandle)
{
    if (m_handle != newHandle)
    {
        m_handle = newHandle;
        emit handleChanged(newHandle);
    }
}

void Shader::setCompileStatus(GLint newCompileStatus)
{
    if (m_compileStatus != newCompileStatus)
    {
        m_compileStatus = newCompileStatus;
        emit compileStatusChanged(newCompileStatus);
    }
}

void Shader::setCompileInfoLog(const QString &newCompileInfoLog)
{
    if (m_compileInfoLog != newCompileInfoLog)
    {
        m_compileInfoLog = newCompileInfoLog;
        emit compileInfoLogChanged(newCompileInfoLog);
    }
}

void Shader::onInit()
{
    setCompileStatus(GL_FALSE);
}

void Shader::onShutdown()
{
    device()->glDeleteShader(handle());
    setHandle(0);
    setCompileStatus(GL_FALSE);
}

void Shader::onUpdate()
{
    if (compileStatus() == GL_TRUE)
    {
        return; // already compiled.
    }

    QByteArray sourceData;

    if (source().startsWith("qrc:", Qt::CaseInsensitive))
    {
        QFile sourceFile(source().mid(3)); // Keep the ':'. This is not a mistake.
        if (sourceFile.open(QFile::ReadOnly | QFile::Text))
        {
            sourceData = sourceFile.readAll();
            sourceFile.close();
        }
        else
        {
            qCritical() << source() << "not found.";
        }
    }
    else if (source().startsWith("file:", Qt::CaseInsensitive))
    {
        QFile sourceFile(QUrl(source()).toLocalFile());
        if (sourceFile.open(QFile::ReadOnly | QFile::Text))
        {
            sourceData = sourceFile.readAll();
            sourceFile.close();
        }
        else
        {
            qCritical() << source() << "not found.";
        }
    }
    else
    {
        sourceData = source().toLocal8Bit();
    }

    const char* sourceCode = sourceData.constData();
    GLint sourceCodeLength = sourceData.size();

    if (sourceCodeLength > 0)
    {
        if (handle())
        {
            device()->glDeleteShader(handle());
        }

        setHandle(device()->glCreateShader(type()));

        device()->glShaderSource(handle(), 1, &sourceCode, &sourceCodeLength);
        device()->glCompileShader(handle());

        GLint compileStatus;
        device()->glGetShaderiv(handle(), GL_COMPILE_STATUS, &compileStatus);
        setCompileStatus(compileStatus);

        if (compileStatus == GL_TRUE)
        {
            setCompileInfoLog(QString());
        }
        else
        {
            GLint infoLogLength = 0;
            device()->glGetShaderiv(handle(), GL_INFO_LOG_LENGTH, &infoLogLength);

            QByteArray infoLog;
            infoLog.resize(infoLogLength);
            device()->glGetShaderInfoLog(handle(), infoLogLength, nullptr, infoLog.data());
            qCritical() << "Could not compile" << source() << QMetaEnum::fromType<Type>().valueToKey(type()) << "shader :" << qPrintable(infoLog);
            setCompileInfoLog(infoLog);
        }
    }
    else
    {
        setCompileStatus(GL_FALSE);
        setCompileInfoLog(QString());
    }
}

