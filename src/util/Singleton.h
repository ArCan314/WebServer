#pragma once

#include "./Noncopyable.h"

template <typename T>
class Singleton : NonCopyable
{
public:
    static T& instance()
    {
        static T obj;
        return obj;
    }
};