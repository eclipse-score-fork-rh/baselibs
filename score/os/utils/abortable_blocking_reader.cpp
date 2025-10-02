/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#include "score/os/utils/abortable_blocking_reader.h"

#include "score/os/fcntl_impl.h"
#include "score/os/sys_poll_impl.h"
#include "score/os/unistd.h"

#include <cerrno>
#include <chrono>
#include <mutex>
#include <thread>
#include <utility>

namespace score
{
namespace os
{

NonBlockingFileDescriptor::~NonBlockingFileDescriptor()
{
    CloseFileDescriptor();
}

NonBlockingFileDescriptor::NonBlockingFileDescriptor(score::os::NonBlockingFileDescriptor&& other) noexcept
    : unistd_{std::move(other.unistd_)}, file_descriptor_{other.file_descriptor_}
{
    other.file_descriptor_ = -1;
}

// Suppress "AUTOSAR C++14 A6-2-1" rule finding. This rule states: "Move and copy assignment operators shall either
// move or respectively copy base classes and data members of a class, without any side effects.".
// Rationale: This operator has side effects - call CloseFileDescriptor, as there is a need close the vaild file
// descriptor if existing which does not support moving.
// coverity[autosar_cpp14_a6_2_1_violation]
NonBlockingFileDescriptor& NonBlockingFileDescriptor::operator=(score::os::NonBlockingFileDescriptor&& other) & noexcept
{
    CloseFileDescriptor();

    unistd_ = std::move(other.unistd_);

    file_descriptor_ = other.file_descriptor_;
    other.file_descriptor_ = -1;

    return *this;
}

score::cpp::expected<NonBlockingFileDescriptor, Error> NonBlockingFileDescriptor::Make(
    const std::int32_t file_descriptor) noexcept
{
    FcntlImpl fcntl_instance{};
    auto unistd = std::make_unique<internal::UnistdImpl>();  // LCOV_EXCL_LINE: tooling issue
    return Make(file_descriptor, fcntl_instance, std::move(unistd));
}

score::cpp::expected<NonBlockingFileDescriptor, Error> NonBlockingFileDescriptor::Make(const std::int32_t file_descriptor,
                                                                                Fcntl& fcntl,
                                                                                std::shared_ptr<Unistd> unistd) noexcept
{
    auto expected_flags = fcntl.fcntl(file_descriptor, Fcntl::Command::kFileGetStatusFlags);
    if (!expected_flags.has_value())
    {
        return score::cpp::make_unexpected(expected_flags.error());
    }
    auto flags = expected_flags.value();

    if (static_cast<std::underlying_type_t<Fcntl::Open>>(flags & Fcntl::Open::kNonBlocking) == 0U)
    {
        flags |= Fcntl::Open::kNonBlocking;
        const auto set_flags = fcntl.fcntl(file_descriptor, Fcntl::Command::kFileSetStatusFlags, flags);

        if (!set_flags.has_value())
        {
            return score::cpp::make_unexpected(set_flags.error());
        }
    }

    return NonBlockingFileDescriptor{file_descriptor, std::move(unistd)};
}

std::int32_t NonBlockingFileDescriptor::GetUnderlying() const noexcept
{
    return file_descriptor_;
}

// Suppress "AUTOSAR C++14 A13-5-2" rule finding: "All user-defined conversion operators shall be defined explicit.”
// Rationale: Intentional implicit conversion to use this class naturally in POSIX API
// coverity[autosar_cpp14_a13_5_2_violation]
NonBlockingFileDescriptor::operator std::int32_t() const noexcept
{
    return file_descriptor_;
}

NonBlockingFileDescriptor::NonBlockingFileDescriptor(const std::int32_t file_descriptor,
                                                     std::shared_ptr<Unistd> unistd) noexcept
    : unistd_{std::move(unistd)}, file_descriptor_{file_descriptor}
{
}

void NonBlockingFileDescriptor::CloseFileDescriptor() noexcept
{
    if ((unistd_ != nullptr) && (file_descriptor_ >= 0))
    {
        // Suppressed here as it is safely used:
        // Provided error check and safeguard to ensure file descriptor is valid.
        // NOLINTNEXTLINE(score-banned-function) see comment above
        const auto result = unistd_->close(file_descriptor_);
        if (!(result.has_value()))
        {
            std::terminate();
        }

        file_descriptor_ = -1;
    }
}

AbortableBlockingReader::AbortableBlockingReader() noexcept
    : AbortableBlockingReader(std::make_shared<FcntlImpl>(),
                              std::make_shared<SysPollImpl>(),
                              std::make_shared<internal::UnistdImpl>())
{
}

AbortableBlockingReader::AbortableBlockingReader(std::shared_ptr<Fcntl> fcntl,
                                                 std::shared_ptr<SysPoll> syspoll,
                                                 std::shared_ptr<Unistd> unistd) noexcept
    : fcntl_{std::move(fcntl)},
      syspoll_{std::move(syspoll)},
      unistd_{std::move(unistd)},
      mutex_{},
      construction_error_{},
      stop_read_file_descriptor_{},
      stop_write_file_descriptor_{}
{
    auto expected_signaling_pipe = MakeNonBlockingPipe(*fcntl_, unistd_);
    if (!(expected_signaling_pipe.has_value()))
    {
        construction_error_ = score::cpp::make_unexpected(expected_signaling_pipe.error());
        return;
    }
    stop_read_file_descriptor_ = std::move(expected_signaling_pipe.value().first);
    stop_write_file_descriptor_ = std::move(expected_signaling_pipe.value().second);
}

AbortableBlockingReader::~AbortableBlockingReader()
{
    Stop();
}

score::cpp::expected_blank<Error> AbortableBlockingReader::IsValid() const noexcept
{
    return construction_error_;
}

void AbortableBlockingReader::Stop() noexcept
{
    std::unique_lock<std::shared_timed_mutex> lock{mutex_, std::defer_lock};

    while (true)
    {
        SignalStop();
        if (lock.try_lock())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }

    // Explicitly reset the file descriptors within the synchronized section
    stop_read_file_descriptor_ = {};
    stop_write_file_descriptor_ = {};
}

score::cpp::expected<score::cpp::span<std::uint8_t>, Error> AbortableBlockingReader::Read(
    const NonBlockingFileDescriptor& file_descriptor,
    const score::cpp::span<std::uint8_t> buffer) noexcept
{
    std::shared_lock<std::shared_timed_mutex> lock{mutex_};

    if (!(IsValid().has_value()))
    {
        return score::cpp::make_unexpected(IsValid().error());
    }

    // While POSIX promises that a select on invalid file descriptors will return with an error, in reality it blocks on
    // some OSs. For this reason, we manually check for already closed file descriptors.
    if ((stop_read_file_descriptor_ == -1) || (file_descriptor.GetUnderlying() == -1))
    {
        return score::cpp::make_unexpected(Error::createFromErrno(EINVAL));
    }

    const auto data_available = WaitForData(file_descriptor);

    if (!data_available.has_value())
    {
        return score::cpp::make_unexpected(data_available.error());
    }

    // Suppressed here as it is safely used:
    // Provided error check and safeguard to ensure file descriptor is valid before reading.
    // NOLINTNEXTLINE(score-banned-function) see comment above
    const auto expected_length = unistd_->read(file_descriptor, buffer.data(), buffer.size());
    if ((!expected_length.has_value()) || (expected_length.value() < 0))
    {
        return score::cpp::make_unexpected(expected_length.error());
    }
    return buffer.first(static_cast<std::size_t>(expected_length.value()));
}

void AbortableBlockingReader::SignalStop() noexcept
{
    if (stop_write_file_descriptor_ < 0)
    {
        return;
    }

    std::array<std::uint8_t, 1U> buffer{};

    // Suppressed here as it is safely used:
    // Provided error check and safeguard to ensure file descriptor is valid before writing.
    // NOLINTNEXTLINE(score-banned-function) see comment above
    const auto result = unistd_->write(stop_write_file_descriptor_, buffer.data(), buffer.size());
    if ((!result.has_value()) && (result.error() != Error::Code::kResourceTemporarilyUnavailable))
    {
        std::terminate();
    }
}

score::cpp::expected_blank<Error> AbortableBlockingReader::WaitForData(
    const NonBlockingFileDescriptor& file_descriptor) noexcept
{
    std::array<struct pollfd, 2> fds{};
    constexpr std::int8_t kNoTimeout{-1};

    fds[0].fd = stop_read_file_descriptor_;
    // Suppress "AUTOSAR C++14 M5-0-21" rule findings. This rule declares: "Bitwise operators shall only be
    // applied to operands of unsigned underlying type."
    // Rationale: Macro POLLIN does not affect the sign of the result.
    // coverity[autosar_cpp14_m5_0_21_violation]
    fds[0].events = POLLIN;

    fds[1].fd = file_descriptor.GetUnderlying();
    // Rationale: see comment above
    // coverity[autosar_cpp14_m5_0_21_violation]
    fds[1].events = POLLIN;

    // coverity[autosar_cpp14_m5_0_6_violation]
    const auto poll_result = syspoll_->poll(fds.data(), fds.size(), kNoTimeout);

    if (!poll_result.has_value())
    {
        return score::cpp::make_unexpected(poll_result.error());
    }

    // Check return event flag for stop_read_file_descriptor_
    // Rationale: see comment above
    // coverity[autosar_cpp14_m5_0_21_violation]
    if ((static_cast<std::uint32_t>(fds[0].revents) & static_cast<std::uint32_t>(POLLIN)) != 0U)
    {
        return score::cpp::make_unexpected(Error::createFromErrno(EINTR));
    }

    return {};
}

score::cpp::expected<std::pair<NonBlockingFileDescriptor, NonBlockingFileDescriptor>, Error>
AbortableBlockingReader::MakeNonBlockingPipe(Fcntl& fcntl, const std::shared_ptr<Unistd>& unistd) noexcept
{
    std::array<std::int32_t, 2> signaling_pipe{};

    // Suppressed here as it is safely used:
    // Provided error check preventing the use of an uninitialized or invalid pipe.
    // NOLINTNEXTLINE(score-banned-function) see comment above
    const auto pipe_result = unistd->pipe(signaling_pipe.data());
    if (!pipe_result.has_value())
    {
        return score::cpp::make_unexpected(pipe_result.error());
    }

    auto expected_reading_file_descriptor = NonBlockingFileDescriptor::Make(signaling_pipe[0], fcntl, unistd);
    if (!expected_reading_file_descriptor.has_value())
    {
        return score::cpp::make_unexpected(expected_reading_file_descriptor.error());
    }

    auto expected_writing_file_descriptor = NonBlockingFileDescriptor::Make(signaling_pipe[1], fcntl, unistd);
    if (!expected_writing_file_descriptor.has_value())
    {
        return score::cpp::make_unexpected(expected_writing_file_descriptor.error());
    }

    return std::make_pair(std::move(expected_reading_file_descriptor.value()),
                          std::move(expected_writing_file_descriptor.value()));
}

}  // namespace os
}  // namespace score
