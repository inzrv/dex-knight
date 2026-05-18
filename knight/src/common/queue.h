#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>

template<typename T>
class IQueue
{
public:
    virtual ~IQueue() = default;
    virtual bool try_push(T&& item) = 0;
    virtual std::optional<T> wait_pop() = 0;
    virtual void close() = 0;
};

template<typename T, size_t Capacity>
class Queue final : public IQueue<T>
{
public:
    bool try_push(T&& item) override
    {
        {
            std::lock_guard lock{m_mutex};

            if (m_closed) {
                return false;
            }

            if (m_queue.size() >= Capacity) {
                return false;
            }

            m_queue.push_back(std::move(item));
        }

        m_cv.notify_one();
        return true;
    }

    std::optional<T> wait_pop() override
    {
        std::unique_lock lock{m_mutex};
        m_cv.wait(lock, [this] {
            return !m_queue.empty() || m_closed;
        });

        if (m_queue.empty()) {
            return std::nullopt;
        }

        T item = std::move(m_queue.front());
        m_queue.pop_front();
        return item;
    }

    void close() override
    {
        {
            std::lock_guard lock{m_mutex};
            m_closed = true;
        }

        m_cv.notify_all();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<T> m_queue;
    bool m_closed{false};
};
