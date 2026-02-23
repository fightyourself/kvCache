#pragma once

class CallDataBase{
public:
    virtual ~CallDataBase() = default;
    virtual void proceed(bool ok) = 0;
};