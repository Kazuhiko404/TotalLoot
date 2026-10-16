#include "order_watch_set.h"

namespace totalloot
{
OrderWatchSet::OrderWatchSet() : m_ownDepth(0) {}

void OrderWatchSet::Watch(const void* actor)
{
    if (actor == NULL)
    {
        return;
    }
    std::map<const void*, Entry>::iterator existing = m_actors.find(actor);
    if (existing != m_actors.end())
    {
        ++existing->second.watchers;
        return;
    }
    Entry entry;
    entry.watchers = 1;
    entry.pending = false;
    entry.last.taskType = 0;
    entry.last.shift = false;
    entry.last.clear = false;
    m_actors[actor] = entry;
}

void OrderWatchSet::Unwatch(const void* actor)
{
    std::map<const void*, Entry>::iterator it = m_actors.find(actor);
    if (it != m_actors.end() && --it->second.watchers <= 0)
    {
        m_actors.erase(it);
    }
}

bool OrderWatchSet::IsWatched(const void* actor) const
{
    return m_actors.find(actor) != m_actors.end();
}

void OrderWatchSet::NoteOrder(const void* actor, int taskType, bool shift, bool clear)
{
    if (m_ownDepth != 0)
    {
        return;
    }
    std::map<const void*, Entry>::iterator it = m_actors.find(actor);
    if (it == m_actors.end())
    {
        return;
    }
    it->second.pending = true;
    it->second.last.taskType = taskType;
    it->second.last.shift = shift;
    it->second.last.clear = clear;
}

bool OrderWatchSet::Take(const void* actor, ForeignOrder* out)
{
    std::map<const void*, Entry>::iterator it = m_actors.find(actor);
    if (it == m_actors.end() || !it->second.pending)
    {
        return false;
    }
    it->second.pending = false;
    if (out != NULL)
    {
        *out = it->second.last;
    }
    return true;
}

void OrderWatchSet::BeginOwn()
{
    ++m_ownDepth;
}

void OrderWatchSet::EndOwn()
{
    if (m_ownDepth > 0)
    {
        --m_ownDepth;
    }
}
} // namespace totalloot
