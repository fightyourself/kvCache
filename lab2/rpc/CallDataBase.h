#pragma once

class CallDataBase{
public:
    virtual ~CallDataBase() = default;
    virtual void Proceed(bool ok) = 0;
};