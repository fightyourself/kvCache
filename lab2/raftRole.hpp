#pragma once

enum class RaftRole{
    FOLLOWER = 1,
    CANDIDATE,
    LEADER
};