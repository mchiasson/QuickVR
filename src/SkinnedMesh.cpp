#include "SkinnedMesh.h"

#include "Device.h"

#include <QMutexLocker>
#include <QtConcurrent>
#include <QGuiApplication>

#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>

#define POSITION_LOCATION    0
#define TEX_COORD_LOCATION   1
#define NORMAL_LOCATION      2
#define BONE_ID_LOCATION     3
#define BONE_WEIGHT_LOCATION 4

#ifndef NDEBUG
static int sceneCount = 0;
#endif

SkinnedMesh::SkinnedMesh(Node* parent) : Node(parent)
{
#ifndef NDEBUG
    if (++sceneCount == 1)
    {
        // Change this line to normal if you not want to analyse the import process
        //Assimp::Logger::LogSeverity severity = Assimp::Logger::NORMAL;
        Assimp::Logger::LogSeverity severity = Assimp::Logger::VERBOSE;

        // Create a logger instance for Console Output
        Assimp::DefaultLogger::create("",severity, aiDefaultLogStream_STDOUT);
    }
#endif

    connect(&m_importerFutureWatcher, &QFutureWatcher<std::shared_ptr<Assimp::Importer>>::finished, this, &SkinnedMesh::onSceneLoadingComplete);
}

SkinnedMesh::~SkinnedMesh()
{
#ifndef NDEBUG
    if (--sceneCount == 0)
    {
        Assimp::DefaultLogger::kill();
    }
#endif
}

void SkinnedMesh::setSource(QString newSource)
{
    if (m_source != newSource)
    {
        m_source = newSource;
        emit sourceChanged(newSource);

        setStatus(Loading);

        // Load the scene from a thread in Qt's thread pool.
        m_importerFutureWatcher.setFuture(QtConcurrent::run([](QString path)->QSharedPointer<Assimp::Importer> {
            QByteArray contentData;
            QFile content;
            if (path.startsWith("qrc:", Qt::CaseInsensitive))
            {
                content.setFileName(path.mid(3)); // Keep the ':'. This is not a mistake.
            }
            else if (path.startsWith("file:", Qt::CaseInsensitive))
            {
                content.setFileName(QUrl(path).toLocalFile());
            }
            else
            {
                content.setFileName(path);
            }

            if (content.open(QFile::ReadOnly))
            {
                contentData = content.readAll();
                content.close();
            }
            else
            {
                qCritical() << path << "not found!";
                return QSharedPointer<Assimp::Importer>();
            }

            QSharedPointer<Assimp::Importer> importer(new Assimp::Importer);
            importer->ReadFileFromMemory(contentData.data(), contentData.size(), aiProcessPreset_TargetRealtime_MaxQuality);
            return importer;

        }, newSource));
    }
}

void SkinnedMesh::setStatus(Status newStatus)
{
    if (m_status != newStatus)
    {
        m_status = newStatus;
        emit statusChanged(newStatus);
    }
}

void SkinnedMesh::onSceneLoadingComplete()
{
    QSharedPointer<Assimp::Importer> importer = m_importerFutureWatcher.result();
    const aiScene *sc = importer->GetScene();
    if (sc)
    {
        setStatus(Processing);
    }
    else
    {
        qCritical() << importer->GetErrorString();
        setStatus(Error);
    }
}

void SkinnedMesh::onInit()
{
    direct_connect(device(), &Device::render, this, &SkinnedMesh::onRender);
}

void SkinnedMesh::onShutdown()
{
    Clear();
}

void SkinnedMesh::onRender()
{
    switch(status())
    {
    case Processing:
    {
        // Release the previously loaded mesh (if it exists)
        Clear();

        // Create the VAO
        device()->glGenVertexArrays(1, &m_VAO);
        device()->glBindVertexArray(m_VAO);

        // Create the buffers for the vertices attributes
        device()->glGenBuffers(ARRAY_SIZE_IN_ELEMENTS(m_Buffers), m_Buffers);

        m_pScene  =  m_importerFutureWatcher.result()->GetScene();

        m_GlobalInverseTransform = glm::inverse(glm::mat4(m_pScene->mRootNode->mTransformation[0][0],m_pScene->mRootNode->mTransformation[0][1],m_pScene->mRootNode->mTransformation[0][2], m_pScene->mRootNode->mTransformation[0][3],
                                                          m_pScene->mRootNode->mTransformation[1][0],m_pScene->mRootNode->mTransformation[1][1],m_pScene->mRootNode->mTransformation[1][2], m_pScene->mRootNode->mTransformation[1][3],
                                                          m_pScene->mRootNode->mTransformation[2][0],m_pScene->mRootNode->mTransformation[2][1],m_pScene->mRootNode->mTransformation[2][2], m_pScene->mRootNode->mTransformation[2][3],
                                                          m_pScene->mRootNode->mTransformation[3][0],m_pScene->mRootNode->mTransformation[3][1],m_pScene->mRootNode->mTransformation[3][2], m_pScene->mRootNode->mTransformation[3][3]));

        InitFromScene(m_pScene);

        // Make sure the VAO is not changed from the outside
        device()->glBindVertexArray(0);

        setStatus(Ready);
        break;
    }
    case Ready:
    {
        if (m_shaderProgram->linkStatus() != GL_TRUE) break;

        m_shaderProgram->bind();

        QVector<glm::mat4> Transforms;
        BoneTransform(device()->t(), Transforms);

        for (int i = 0 ; i < Transforms.size() ; i++) {
            m_shaderProgram->setBoneTransform(i, Transforms[i]);
        }

        m_shaderProgram->setEyeWorldPos(device()->eyePosition());

        // Combine the transformations (TRS)
        glm::mat4 Model = glm::scale(glm::rotate(glm::translate(glm::mat4(1), glm::vec3(x(), y(), z())), glm::radians((float)rotation()), glm::vec3(0,1,0)), glm::vec3(scale(), scale(), scale()));

        glm::mat4 MVP = device()->proj() * device()->view() * Model;

        m_shaderProgram->setMVP(MVP);
        m_shaderProgram->setModel(Model);

        device()->glBindVertexArray(m_VAO);

        for (int32_t i = 0 ; i < m_Entries.size() ; i++)
        {
            const int32_t MaterialIndex = m_Entries[i].MaterialIndex;

            assert(MaterialIndex < m_Textures.size());

            if (m_Textures[MaterialIndex])
            {
                m_Textures[MaterialIndex]->bind(0);
            }

            device()->glDrawElementsBaseVertex(GL_TRIANGLES, m_Entries[i].NumIndices, GL_UNSIGNED_INT, (void*)(sizeof(uint) * m_Entries[i].BaseIndex), m_Entries[i].BaseVertex);
        }

        device()->glBindVertexArray(0);
        break;
    }
    default:
        break;
    }
}

void SkinnedMesh::VertexBoneData::AddBoneData(uint32_t BoneID, float Weight)
{
    for (size_t i = 0 ; i < ARRAY_SIZE_IN_ELEMENTS(IDs) ; i++)
    {
        if (Weights[i] == 0.0)
        {
            IDs[i]     = BoneID;
            Weights[i] = Weight;
            return;
        }
    }

    // should never get here
    qFatal("reached more bones than we have space for.");
}

void SkinnedMesh::Clear()
{
    for (int32_t i = 0 ; i < m_Textures.size() ; i++)
    {
        SAFE_DELETE(m_Textures[i]);
    }

    if (m_Buffers[0] != 0)
    {
        device()->glDeleteBuffers(ARRAY_SIZE_IN_ELEMENTS(m_Buffers), m_Buffers);
    }

    if (m_VAO != 0)
    {
        device()->glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
}

bool SkinnedMesh::InitFromScene(const aiScene* pScene)
{
    m_Entries.resize(pScene->mNumMeshes);
    m_Textures.resize(pScene->mNumMaterials);

    QVector<glm::vec3> Positions;
    QVector<glm::vec3> Normals;
    QVector<QVector2D> TexCoords;
    QVector<VertexBoneData> Bones;
    QVector<uint32_t> Indices;

    uint32_t NumVertices = 0;
    uint32_t NumIndices = 0;

    // Count the number of vertices and indices
    for (int32_t i = 0 ; i < m_Entries.size() ; i++)
    {
        m_Entries[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;
        m_Entries[i].NumIndices    = pScene->mMeshes[i]->mNumFaces * 3;
        m_Entries[i].BaseVertex    = NumVertices;
        m_Entries[i].BaseIndex     = NumIndices;

        NumVertices += pScene->mMeshes[i]->mNumVertices;
        NumIndices  += m_Entries[i].NumIndices;
    }

    // Reserve space in the vectors for the vertex attributes and indices
    Positions.reserve(NumVertices);
    Normals.reserve(NumVertices);
    TexCoords.reserve(NumVertices);
    Bones.resize(NumVertices);
    Indices.reserve(NumIndices);

    // Initialize the meshes in the scene one by one
    for (int32_t i = 0 ; i < m_Entries.size() ; i++)
    {
        const aiMesh* paiMesh = pScene->mMeshes[i];
        InitMesh(i, paiMesh, Positions, Normals, TexCoords, Bones, Indices);
    }

    if (!InitMaterials(pScene))
    {
        return false;
    }

    // Generate and populate the buffers with vertex attributes and the indices
    device()->glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]);
    device()->glBufferData(GL_ARRAY_BUFFER, sizeof(Positions[0]) * Positions.size(), &Positions[0], GL_STATIC_DRAW);
    device()->glEnableVertexAttribArray(POSITION_LOCATION);
    device()->glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

    device()->glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]);
    device()->glBufferData(GL_ARRAY_BUFFER, sizeof(TexCoords[0]) * TexCoords.size(), &TexCoords[0], GL_STATIC_DRAW);
    device()->glEnableVertexAttribArray(TEX_COORD_LOCATION);
    device()->glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

    device()->glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
    device()->glBufferData(GL_ARRAY_BUFFER, sizeof(Normals[0]) * Normals.size(), &Normals[0], GL_STATIC_DRAW);
    device()->glEnableVertexAttribArray(NORMAL_LOCATION);
    device()->glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

    device()->glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[BONE_VB]);
    device()->glBufferData(GL_ARRAY_BUFFER, sizeof(Bones[0]) * Bones.size(), &Bones[0], GL_STATIC_DRAW);
    device()->glEnableVertexAttribArray(BONE_ID_LOCATION);
    device()->glVertexAttribIPointer(BONE_ID_LOCATION, 4, GL_INT, sizeof(VertexBoneData), (const GLvoid*)0);
    device()->glEnableVertexAttribArray(BONE_WEIGHT_LOCATION);
    device()->glVertexAttribPointer(BONE_WEIGHT_LOCATION, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (const GLvoid*)16);

    device()->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
    device()->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices[0]) * Indices.size(), &Indices[0], GL_STATIC_DRAW);

    return true;
}

void SkinnedMesh::InitMesh(uint MeshIndex,
                           const aiMesh* paiMesh,
                           QVector<glm::vec3>& Positions,
                           QVector<glm::vec3>& Normals,
                           QVector<QVector2D>& TexCoords,
                           QVector<VertexBoneData>& Bones,
                           QVector<uint32_t>& Indices)
{
    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

    for (uint32_t i = 0 ; i < paiMesh->mNumVertices ; i++)
    {
        const aiVector3D* pPos      = &(paiMesh->mVertices[i]);
        const aiVector3D* pNormal   = &(paiMesh->mNormals[i]);
        const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;

        Positions.push_back(glm::vec3(pPos->x, pPos->y, pPos->z));
        Normals.push_back(glm::vec3(pNormal->x, pNormal->y, pNormal->z));
        TexCoords.push_back(QVector2D(pTexCoord->x, pTexCoord->y));
    }

    LoadBones(MeshIndex, paiMesh, Bones);

    // Populate the index buffer
    for (uint i = 0 ; i < paiMesh->mNumFaces ; i++) {
        const aiFace& Face = paiMesh->mFaces[i];
        assert(Face.mNumIndices == 3);
        Indices.push_back(Face.mIndices[0]);
        Indices.push_back(Face.mIndices[1]);
        Indices.push_back(Face.mIndices[2]);
    }
}

void SkinnedMesh::LoadBones(uint MeshIndex, const aiMesh* pMesh, QVector<VertexBoneData>& Bones)
{
    for (uint32_t i = 0 ; i < pMesh->mNumBones ; i++) {
        uint32_t BoneIndex = 0;
        QString BoneName(pMesh->mBones[i]->mName.data);

        if (m_BoneMapping.find(BoneName) == m_BoneMapping.end()) {
            // Allocate an index for a new bone
            BoneIndex = m_NumBones;
            m_NumBones++;
            BoneInfo bi;
            m_BoneInfo.push_back(bi);
            m_BoneInfo[BoneIndex].BoneOffset = glm::mat4(pMesh->mBones[i]->mOffsetMatrix[0][0],pMesh->mBones[i]->mOffsetMatrix[0][1],pMesh->mBones[i]->mOffsetMatrix[0][2],pMesh->mBones[i]->mOffsetMatrix[0][3],
                                                         pMesh->mBones[i]->mOffsetMatrix[1][0],pMesh->mBones[i]->mOffsetMatrix[1][1],pMesh->mBones[i]->mOffsetMatrix[1][2],pMesh->mBones[i]->mOffsetMatrix[1][3],
                                                         pMesh->mBones[i]->mOffsetMatrix[2][0],pMesh->mBones[i]->mOffsetMatrix[2][1],pMesh->mBones[i]->mOffsetMatrix[2][2],pMesh->mBones[i]->mOffsetMatrix[2][3],
                                                         pMesh->mBones[i]->mOffsetMatrix[3][0],pMesh->mBones[i]->mOffsetMatrix[3][1],pMesh->mBones[i]->mOffsetMatrix[3][2],pMesh->mBones[i]->mOffsetMatrix[3][3]);
            m_BoneMapping[BoneName] = BoneIndex;
        }
        else {
            BoneIndex = m_BoneMapping[BoneName];
        }

        for (uint32_t j = 0 ; j < pMesh->mBones[i]->mNumWeights ; j++) {
            uint32_t VertexID = m_Entries[MeshIndex].BaseVertex + pMesh->mBones[i]->mWeights[j].mVertexId;
            float Weight  = pMesh->mBones[i]->mWeights[j].mWeight;
            Bones[VertexID].AddBoneData(BoneIndex, Weight);
        }
    }
}

bool SkinnedMesh::InitMaterials(const aiScene* pScene)
{
    // Extract the directory part from the file name
    int32_t SlashIndex = m_source.lastIndexOf("/");
    QString Dir;

    if (SlashIndex == -1) {
        Dir = ".";
    }
    else if (SlashIndex == 0) {
        Dir = "/";
    }
    else {
        Dir = m_source.left(SlashIndex);
    }

    bool Ret = true;

    // Initialize the materials
    for (uint i = 0 ; i < pScene->mNumMaterials ; i++) {
        const aiMaterial* pMaterial = pScene->mMaterials[i];

        m_Textures[i] = NULL;

        if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString Path;

            if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
                QString p(Path.data);

                if (p.left(2) == ".\\") {
                    p = p.mid(2);
                }

                QString FullPath = Dir + "/" + p;

                m_Textures[i] = new Texture();
                m_Textures[i]->setDevice(device());

                if (!m_Textures[i]->load(FullPath)) {
                    qCritical("Error loading texture '%s'\n", qPrintable(FullPath));
                    delete m_Textures[i];
                    m_Textures[i] = NULL;
                    Ret = false;
                }
                else {
                    qCritical("%d - loaded texture '%s'\n", i, qPrintable(FullPath));
                }
            }
        }
    }

    return Ret;
}

uint SkinnedMesh::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    for (uint i = 0 ; i < pNodeAnim->mNumPositionKeys - 1 ; i++) {
        if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
            return i;
        }
    }

    assert(0);

    return 0;
}


uint SkinnedMesh::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumRotationKeys > 0);

    for (uint i = 0 ; i < pNodeAnim->mNumRotationKeys - 1 ; i++) {
        if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
            return i;
        }
    }

    assert(0);

    return 0;
}


uint SkinnedMesh::FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumScalingKeys > 0);

    for (uint i = 0 ; i < pNodeAnim->mNumScalingKeys - 1 ; i++) {
        if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
            return i;
        }
    }

    assert(0);

    return 0;
}


void SkinnedMesh::CalcInterpolatedPosition(glm::vec3& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumPositionKeys == 1) {
        Out = glm::vec3(pNodeAnim->mPositionKeys[0].mValue.x, pNodeAnim->mPositionKeys[0].mValue.y, pNodeAnim->mPositionKeys[0].mValue.z);
        return;
    }

    uint PositionIndex = FindPosition(AnimationTime, pNodeAnim);
    uint NextPositionIndex = (PositionIndex + 1);
    assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
    float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
    float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const glm::vec3 Start(pNodeAnim->mPositionKeys[PositionIndex].mValue.x, pNodeAnim->mPositionKeys[PositionIndex].mValue.y, pNodeAnim->mPositionKeys[PositionIndex].mValue.z);
    const glm::vec3 End(pNodeAnim->mPositionKeys[NextPositionIndex].mValue.x, pNodeAnim->mPositionKeys[NextPositionIndex].mValue.y, pNodeAnim->mPositionKeys[NextPositionIndex].mValue.z);
    glm::vec3 Delta = End - Start;
    Out = Start + Factor * Delta;
}


void SkinnedMesh::CalcInterpolatedRotation(glm::quat& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumRotationKeys == 1) {
        Out = glm::quat(pNodeAnim->mRotationKeys[0].mValue.w, pNodeAnim->mRotationKeys[0].mValue.x, pNodeAnim->mRotationKeys[0].mValue.y, pNodeAnim->mRotationKeys[0].mValue.z);
        return;
    }

    uint RotationIndex = FindRotation(AnimationTime, pNodeAnim);
    uint NextRotationIndex = (RotationIndex + 1);
    assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
    float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
    float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const glm::quat StartRotationQ(pNodeAnim->mRotationKeys[RotationIndex].mValue.w, pNodeAnim->mRotationKeys[RotationIndex].mValue.x, pNodeAnim->mRotationKeys[RotationIndex].mValue.y, pNodeAnim->mRotationKeys[RotationIndex].mValue.z);
    const glm::quat EndRotationQ(pNodeAnim->mRotationKeys[NextRotationIndex].mValue.w, pNodeAnim->mRotationKeys[NextRotationIndex].mValue.x, pNodeAnim->mRotationKeys[NextRotationIndex].mValue.y, pNodeAnim->mRotationKeys[NextRotationIndex].mValue.z);
    Out = glm::normalize(glm::slerp(StartRotationQ, EndRotationQ, Factor));
}


void SkinnedMesh::CalcInterpolatedScaling(glm::vec3& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumScalingKeys == 1) {
        Out = glm::vec3(pNodeAnim->mScalingKeys[0].mValue.x, pNodeAnim->mScalingKeys[0].mValue.y, pNodeAnim->mScalingKeys[0].mValue.z);
        return;
    }

    uint ScalingIndex = FindScaling(AnimationTime, pNodeAnim);
    uint NextScalingIndex = (ScalingIndex + 1);
    assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
    float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
    float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const glm::vec3 Start(pNodeAnim->mScalingKeys[ScalingIndex].mValue.x, pNodeAnim->mScalingKeys[ScalingIndex].mValue.y, pNodeAnim->mScalingKeys[ScalingIndex].mValue.z);
    const glm::vec3 End(pNodeAnim->mScalingKeys[NextScalingIndex].mValue.x, pNodeAnim->mScalingKeys[NextScalingIndex].mValue.y, pNodeAnim->mScalingKeys[NextScalingIndex].mValue.z);
    glm::vec3 Delta = End - Start;
    Out = Start + Factor * Delta;
}


void SkinnedMesh::ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform)
{
    QString NodeName(pNode->mName.data);

    const aiAnimation* pAnimation = m_pScene->mAnimations[0];

    glm::mat4 NodeTransformation(pNode->mTransformation[0][0],pNode->mTransformation[0][1],pNode->mTransformation[0][2],pNode->mTransformation[0][3],
                                 pNode->mTransformation[1][0],pNode->mTransformation[1][1],pNode->mTransformation[1][2],pNode->mTransformation[1][3],
                                 pNode->mTransformation[2][0],pNode->mTransformation[2][1],pNode->mTransformation[2][2],pNode->mTransformation[2][3],
                                 pNode->mTransformation[3][0],pNode->mTransformation[3][1],pNode->mTransformation[3][2],pNode->mTransformation[3][3]);

    const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);

    if (pNodeAnim) {
        // Interpolate scaling and generate scaling transformation matrix
        glm::vec3 Scaling;
        CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);

        // Interpolate rotation and generate rotation transformation matrix
        glm::quat RotationQ;
        CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);

        // Interpolate translation and generate translation transformation matrix
        glm::vec3 Translation;
        CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);

        // Combine the above transformations (TRS)
        NodeTransformation = glm::scale(glm::translate(glm::mat4(NodeTransformation), Translation) * glm::mat4(RotationQ), Scaling);

    }

    glm::mat4 GlobalTransformation = ParentTransform * NodeTransformation;

    if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
        uint BoneIndex = m_BoneMapping[NodeName];
        m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
    }

    for (uint i = 0 ; i < pNode->mNumChildren ; i++) {
        ReadNodeHeirarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
    }
}

void SkinnedMesh::setShaderProgram(ShaderProgram *newShaderProgram)
{
    if (m_shaderProgram != newShaderProgram)
    {
        m_shaderProgram = newShaderProgram;
        emit shaderProgramChanged(newShaderProgram);
    }
}

void SkinnedMesh::BoneTransform(float TimeInSeconds, QVector<glm::mat4>& Transforms)
{
    if (!m_pScene->HasAnimations()) return;

    glm::mat4 Identity;

    float TicksPerSecond = (float)(m_pScene->mAnimations[0]->mTicksPerSecond != 0 ? m_pScene->mAnimations[0]->mTicksPerSecond : 25.0f);
    float TimeInTicks = TimeInSeconds * TicksPerSecond;
    float AnimationTime = fmod(TimeInTicks, (float)m_pScene->mAnimations[0]->mDuration);

    ReadNodeHeirarchy(AnimationTime, m_pScene->mRootNode, Identity);

    Transforms.resize(m_NumBones);

    for (uint i = 0 ; i < m_NumBones ; i++) {
        Transforms[i] = m_BoneInfo[i].FinalTransformation;
    }
}


const aiNodeAnim* SkinnedMesh::FindNodeAnim(const aiAnimation* pAnimation, const QString &NodeName)
{
    for (uint32_t i = 0 ; i < pAnimation->mNumChannels ; i++) {
        const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

        if (NodeName == pNodeAnim->mNodeName.data) {
            return pNodeAnim;
        }
    }

    return nullptr;
}
