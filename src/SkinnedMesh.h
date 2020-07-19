#ifndef SKINNEDMESH_H
#define SKINNEDMESH_H

#include "Node.h"
#include "Texture.h"
#include "ShaderProgram.h"

#include <memory>
#include <cstring>

#include <QFutureWatcher>
#include <QSharedPointer>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define NUM_BONES_PER_VEREX 4
#define INVALID_MATERIAL 0xFFFFFFFF



class SkinnedMesh : public Node
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(ShaderProgram* shaderProgram READ shaderProgram WRITE setShaderProgram NOTIFY shaderProgramChanged)
public:

    enum Status
    {
        None = 0,
        Loading,
        Processing,
        Ready,
        Error
    };
    Q_ENUMS(Status)

    explicit SkinnedMesh(Node* parent = nullptr);
    virtual ~SkinnedMesh() override;

    QString source() const { return m_source; }
    void setSource(QString newSource);

    Status status() const { return m_status; }
    void setStatus(Status newStatus);

    ShaderProgram *shaderProgram() const { return m_shaderProgram; }
    void setShaderProgram(ShaderProgram *newShaderProgram);

    void BoneTransform(float TimeInSeconds, QVector<glm::mat4>& Transforms);

signals:

    void sourceChanged(QString newSource);
    void statusChanged(Status newStatus);
    void shaderProgramChanged(ShaderProgram *newShaderProgram);

protected slots:

    void onSceneLoadingComplete();

protected:

    virtual void onInit() override;
    virtual void onShutdown() override;
    void onRender();

    QString m_source;
    Status m_status = None;

    QFutureWatcher<QSharedPointer<Assimp::Importer>> m_importerFutureWatcher;
    const aiScene *m_pScene = nullptr;

    struct BoneInfo
    {
        glm::mat4 BoneOffset;
        glm::mat4 FinalTransformation;
    };

    struct VertexBoneData
    {
        uint32_t IDs[NUM_BONES_PER_VEREX]  = {};
        float Weights[NUM_BONES_PER_VEREX] = {};

        void Reset()
        {
            ZERO_MEM(IDs);
            ZERO_MEM(Weights);
        }

        void AddBoneData(uint32_t BoneID, float Weight);
    };

    void Clear();
    void CalcInterpolatedScaling(glm::vec3& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedRotation(glm::quat& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedPosition(glm::vec3& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    uint32_t FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
    uint32_t FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
    uint32_t FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
    const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const QString &NodeName);
    void ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform);
    bool InitFromScene(const aiScene* pScene);
    void InitMesh(uint32_t MeshIndex,
                  const aiMesh* paiMesh,
                  QVector<glm::vec3>& Positions,
                  QVector<glm::vec3>& Normals,
                  QVector<QVector2D>& TexCoords,
                  QVector<VertexBoneData>& Bones,
                  QVector<uint32_t>& Indices);
    void LoadBones(uint32_t MeshIndex, const aiMesh* paiMesh, QVector<VertexBoneData>& Bones);
    bool InitMaterials(const aiScene* pScene);

    enum VB_TYPES {
        INDEX_BUFFER,
        POS_VB,
        NORMAL_VB,
        TEXCOORD_VB,
        BONE_VB,
        NUM_VBs
    };

    GLuint m_VAO = 0;
    GLuint m_Buffers[NUM_VBs] = {};

    struct MeshEntry {
        MeshEntry()
        {
            NumIndices    = 0;
            BaseVertex    = 0;
            BaseIndex     = 0;
            MaterialIndex = INVALID_MATERIAL;
        }

        uint32_t NumIndices;
        uint32_t BaseVertex;
        uint32_t BaseIndex;
        int32_t MaterialIndex;
    };

    QVector<MeshEntry> m_Entries;
    QVector<Texture*> m_Textures;

    QHash<QString, uint32_t> m_BoneMapping; // maps a bone name to its index
    uint32_t m_NumBones = 0;
    QVector<BoneInfo> m_BoneInfo;
    glm::mat4 m_GlobalInverseTransform;

    ShaderProgram *m_shaderProgram = nullptr;
};


#endif // SKINNEDMESH_H
