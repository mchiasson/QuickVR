#ifndef VERTEXATTRIBUTE_H
#define VERTEXATTRIBUTE_H

#include "Node.h"

#include "Buffer.h"
#include <QOpenGLExtraFunctions>

class VertexAttribute : public Node
{
    Q_OBJECT
    Q_PROPERTY(QString name       READ name       WRITE setName       NOTIFY nameChanged)
    Q_PROPERTY(int     size       READ size       WRITE setSize       NOTIFY sizeChanged)
    Q_PROPERTY(Type    type       READ type       WRITE setType       NOTIFY typeChanged)
    Q_PROPERTY(bool    normalized READ normalized WRITE setNormalized NOTIFY normalizedChanged)
    Q_PROPERTY(int     stride     READ stride     WRITE setStride     NOTIFY strideChanged)
    Q_PROPERTY(int     offset     READ offset     WRITE setOffset     NOTIFY offsetChanged)
    Q_PROPERTY(Buffer* buffer     READ Buffer     WRITE setOffset     NOTIFY offsetChanged)

public:

    enum Type
    {
        TypeByte          = GL_BYTE,
        TypeUnsignedByte  = GL_UNSIGNED_BYTE,
        TypeShort         = GL_SHORT,
        TypeUnsignedShort = GL_UNSIGNED_SHORT,
        TypeInt           = GL_INT,
        TypeUnsignedInt   = GL_UNSIGNED_INT,
        TypeHalfFloat     = GL_HALF_FLOAT,
        TypeFloat         = GL_FLOAT,
        TypeDouble        = GL_DOUBLE
    };
    Q_ENUM(Type)

    explicit VertexAttribute(Node *parent = nullptr) : Node(parent) {}
};

#endif // VERTEXATTRIBUTE_H
