#include "SIGSCore.h"
#include <algorithm>

namespace sigs::core
{
    void FrameStream::feed(const Bytes& input, const std::function<void(const Packet&)>& emit)
    {
        // Receive boundaries are unrelated to frames: retain fragments and continue through coalesced packets.
        std::size_t inputOffset = 0;
        while (inputOffset < input.size())
        {
            auto bytesToCopy = std::min(expected_ - buffer_.size(), input.size() - inputOffset);
            buffer_.insert(buffer_.end(), input.begin() + inputOffset, input.begin() + inputOffset + bytesToCopy);
            inputOffset += bytesToCopy;
            if (buffer_.size() != expected_)
            {
                continue;
            }
            if (expected_ == 4)
            {
                auto frameLength = read_be32(buffer_, 0);
                if (frameLength < 12 || frameLength > MaxFrame)
                {
                    throw std::runtime_error("Invalid frame size");
                }
                expected_ = std::size_t(frameLength) + 4;
            }
            else
            {
                auto packet = decode_packet(buffer_);
                // Consume before invoking application code so callback failure cannot cause a packet to be replayed.
                buffer_.clear();
                expected_ = 4;
                emit(packet);
            }
        }
    }

    void FrameStream::finish() const
    {
        if (!buffer_.empty())
        {
            throw std::runtime_error("EOF mid-frame");
        }
    }
} // namespace sigs::core
