#include "Buffer.h"

#include "Device.h"
#include <QMutexLocker>

void Buffer::setTarget(Target newTarget)
{
    QMutexLocker lock(&m_mutex);
    if (m_target != newTarget)
    {
        m_target = newTarget;
        m_dirtyFlag |= DirtyTarget;
        emit targetChanged(newTarget);
    }
}

void Buffer::setUsage(Usage newUsage)
{
    QMutexLocker lock(&m_mutex);
    if (m_usage != newUsage)
    {
        m_usage = newUsage;
        m_dirtyFlag |= DirtyUsage;
        emit usageChanged(newUsage);
    }
}

void Buffer::setData(const QByteArray &newData)
{
    QMutexLocker lock(&m_mutex);
    m_data = newData;
    m_dirtyFlag |= DirtyData;
    emit dataChanged(newData);
}

void Buffer::bind()
{
    if (handle())
    {
        device()->glBindBuffer(target(), handle());
    }
}

void Buffer::onUpdate()
{
    if (m_dirtyFlag == DirtyNone)
        return;

    QMutexLocker lock(&m_mutex);

    if (!handle())
    {
        device()->glGenBuffers(1, &m_handle);
    }

    if (m_dirtyFlag & (DirtyTarget|DirtyData|DirtyUsage))
    {
        device()->glBindBuffer(target(), handle());
        device()->glBufferData(target(), m_data.size(), m_data.data(), usage());
        m_dirtyFlag &= ~(DirtyTarget|DirtyData|DirtyUsage);
    }
}

void Buffer::onShutdown()
{
    if (handle())
    {
        device()->glDeleteBuffers(1, &m_handle);
        m_handle = 0;
    }
}
