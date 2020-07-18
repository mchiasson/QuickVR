#ifndef SHADER_H
#define SHADER_H

#include <QOpenGLExtraFunctions>

#include "Node.h"

class Shader : public Node
{
    Q_OBJECT
    Q_PROPERTY(QString source         READ source         WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(Type    type           READ type           WRITE setType   NOTIFY typeChanged)
    Q_PROPERTY(GLuint  handle         READ handle                         NOTIFY handleChanged)
    Q_PROPERTY(bool    compileStatus  READ compileStatus                  NOTIFY compileStatusChanged)
    Q_PROPERTY(QString compileInfoLog READ compileInfoLog                 NOTIFY compileInfoLogChanged)

public:
    enum Type
    {
        Vertex = GL_VERTEX_SHADER,
        Fragment = GL_FRAGMENT_SHADER
    };
    Q_ENUM(Type)

    explicit Shader(Node *parent = nullptr) : Node(parent) { }

    const QString &source() const  { return m_source; }
    Type type() const { return (Type)m_type; }
    GLuint handle() const { return m_handle; }
    bool compileStatus() const { return m_compileStatus == GL_TRUE; }
    const QString &compileInfoLog() const { return m_compileInfoLog; }

    void setSource(const QString &newSource);
    void setType(Type newType);
    void setHandle(GLuint newHandle);
    void setCompileStatus(GLint newCompileStatus);
    void setCompileInfoLog(const QString &newCompileInfoLog);

signals:

    void sourceChanged(const QString &newSource);
    void typeChanged(Type newType);
    void handleChanged(GLuint handle);
    void compileStatusChanged(bool newCompileStatus);
    void compileInfoLogChanged(const QString &newCompileInfoLog);

protected:

    virtual void onInit() override;
    virtual void onShutdown() override;
    virtual void onUpdate() override;

protected:
    QString m_source;
    GLenum m_type = Vertex;
    GLint m_compileStatus = GL_FALSE;
    QString m_compileInfoLog;
    GLuint m_handle = 0;
};

#endif /* SHADER_H */

