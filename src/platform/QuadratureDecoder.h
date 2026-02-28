#pragma once

/// Pure-logic quadrature decoder for rotary encoders.
/// Uses a Gray code state machine lookup table to determine rotation direction
/// from two-channel (A/B) state transitions.
///
/// Returns +1 for CW, -1 for CCW, 0 for invalid/no-change.
/// No hardware or library dependencies — testable on all platforms.
class QuadratureDecoder
{
public:
    /// Feed new channel values. Returns direction: +1 (CW), -1 (CCW), 0 (invalid/same).
    int update(int channelA, int channelB)
    {
        int newState = (channelA << 1) | channelB;
        int direction = kTable[m_prevState][newState];
        m_prevState = newState;
        return direction;
    }

    /// Initialize decoder state from current pin values (call before entering event loop).
    void reset(int channelA, int channelB) { m_prevState = (channelA << 1) | channelB; }

private:
    int m_prevState = 0;

    // Lookup: kTable[prevState][newState] -> direction
    // State encoding: (channelA << 1) | channelB
    //   00=0, 01=1, 10=2, 11=3
    // CW sequence:  00(0) -> 01(1) -> 11(3) -> 10(2) -> 00(0)  (each step = +1)
    // CCW sequence: 00(0) -> 10(2) -> 11(3) -> 01(1) -> 00(0)  (each step = -1)
    // Same state or invalid jump = 0
    static constexpr int kTable[4][4] = {
        //        00   01   10   11
        /* 00 */ { 0, +1, -1, 0 },
        /* 01 */ { -1, 0, 0, +1 },
        /* 10 */ { +1, 0, 0, -1 },
        /* 11 */ { 0, -1, +1, 0 },
    };
};
