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
#include "score/os/utils/inotify/inotify_instance_impl.h"
#include "score/os/fcntl_impl.h"
#include "score/os/inotify_impl.h"
#include "score/os/sys_poll_impl.h"
#include "score/os/unistd.h"

#include "score/language/safecpp/string_view/null_termination_check.h"

namespace score
{
namespace os
{

InotifyInstanceImpl::InotifyInstanceImpl() noexcept
    : InotifyInstanceImpl{std::make_shared<InotifyImpl>(),
                          std::make_shared<FcntlImpl>(),
                          std::make_shared<SysPollImpl>(),
                          std::make_shared<internal::UnistdImpl>()}
{
}

InotifyInstanceImpl::InotifyInstanceImpl(const std::shared_ptr<Inotify>& inotify,
                                         const std::shared_ptr<Fcntl>& fcntl,
                                         const std::shared_ptr<SysPoll>& syspoll,
                                         const std::shared_ptr<Unistd>& unistd) noexcept
    : InotifyInstance{},
      inotify_{inotify},
      construction_error_{},
      inotify_file_descriptor_{},
      reader_{fcntl, syspoll, unistd},
      inotify_file_descriptor_mutex_{}
{
    auto expected_inotify_file_descriptor = InitializeInotify(*inotify_, *fcntl, unistd);
    if (!(expected_inotify_file_descriptor.has_value()))
    {
        construction_error_ = score::cpp::make_unexpected(expected_inotify_file_descriptor.error());
        return;
    }
    inotify_file_descriptor_ = std::move(expected_inotify_file_descriptor.value());

    const auto valid_reader = reader_.IsValid();
    if (!(valid_reader.has_value()))
    {
        construction_error_ = score::cpp::make_unexpected(valid_reader.error());
        return;
    }
}

InotifyInstanceImpl::~InotifyInstanceImpl() noexcept
{
    InternalClose();
}

score::cpp::expected_blank<Error> InotifyInstanceImpl::IsValid() const noexcept
{
    return construction_error_;
}

void InotifyInstanceImpl::Close() noexcept
{
    InternalClose();
}

void InotifyInstanceImpl::InternalClose() noexcept
{
    reader_.Stop();
    {
        const std::lock_guard<std::shared_timed_mutex> lock{inotify_file_descriptor_mutex_};
        inotify_file_descriptor_ = {};
    }
}

score::cpp::expected<InotifyWatchDescriptor, Error> InotifyInstanceImpl::AddWatch(std::string_view pathname,
                                                                           Inotify::EventMask event_mask) noexcept
{
    if (!(IsValid().has_value()))
    {
        return score::cpp::make_unexpected(Error::createFromErrno(EINVAL));
    }

    {
        std::shared_lock<std::shared_timed_mutex> lock{inotify_file_descriptor_mutex_};
        // NOTE: Below use of `safecpp::GetPtrToNullTerminatedUnderlyingBufferOf()` will emit a deprecation warning here
        //       since it is used in conjunction with `std::string_view`. Thus, it must get fixed appropriately instead!
        //       For examples about how to achieve that, see
        //       broken_link_g/swh/safe-posix-platform/blob/master/score/language/safecpp/string_view/README.md
        const auto expected_watch_descriptor = inotify_->inotify_add_watch(
            inotify_file_descriptor_, safecpp::GetPtrToNullTerminatedUnderlyingBufferOf(pathname), event_mask);
        if (!expected_watch_descriptor.has_value())
        {
            return score::cpp::make_unexpected(expected_watch_descriptor.error());
        }

        return InotifyWatchDescriptor{expected_watch_descriptor.value()};
    }
}

score::cpp::expected_blank<Error> InotifyInstanceImpl::RemoveWatch(InotifyWatchDescriptor watch_descriptor) noexcept
{
    if (!(IsValid().has_value()))
    {
        return score::cpp::make_unexpected(Error::createFromErrno(EINVAL));
    }

    {
        std::shared_lock<std::shared_timed_mutex> lock{inotify_file_descriptor_mutex_};
        const auto result = inotify_->inotify_rm_watch(inotify_file_descriptor_, watch_descriptor.GetUnderlying());
        if (!result.has_value())
        {
            return score::cpp::make_unexpected(result.error());
        }
    }

    return {};
}

score::cpp::expected<score::cpp::static_vector<InotifyEvent, InotifyInstanceImpl::max_events>, Error>
InotifyInstanceImpl::Read() noexcept
{
    if (!(IsValid().has_value()))
    {
        return score::cpp::make_unexpected(Error::createFromErrno(EINVAL));
    }

    // Suppress "AUTOSAR C++14 A0-1-1" rule finding. This rule states: "A project shall not contain instances of
    // non-volatile variables being given values that are not subsequently used".
    // The variable buffer_size is used here for std::array signature declaration.
    // coverity[autosar_cpp14_a0_1_1_violation]
    constexpr auto buffer_size = max_events * sizeof(struct inotify_event);
    alignas(struct inotify_event) std::array<std::uint8_t, buffer_size> buffer{};
    std::shared_lock<std::shared_timed_mutex> lock{inotify_file_descriptor_mutex_};
    const auto expected_event_buffer = reader_.Read(inotify_file_descriptor_, buffer);
    lock.unlock();
    if (!expected_event_buffer.has_value())
    {
        return score::cpp::make_unexpected(expected_event_buffer.error());
    }
    const score::cpp::span<std::uint8_t> event_buffer{expected_event_buffer.value()};

    score::cpp::static_vector<InotifyEvent, max_events> events{};

    // Suppress “AUTOSAR C++14 A6-5-2” rule finding: “A for loop shall contain a single loop-counter which shall not
    // have floating-point type.”
    // Rationale:As event_buffer is of type score::cpp::span<std::uint8_t>, which does not support
    // iteration by offsets or provides a mechanism to expose ranges. No harm to functionality
    // coverity[autosar_cpp14_a6_5_2_violation]
    for (auto it = event_buffer.cbegin(); it < event_buffer.cend();)
    {
        // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast) see rationale below
        // Suppress “AUTOSAR_Cpp14_A5_2_4” rule finding: “Reinterpret_cast shall not be used.”
        // Rationale: Reinterpret_cast required to access the raw data from a span as a structured inotify_event.
        // Correct alignment and sufficient space for storing inotify_event objects is provided.
        // coverity[autosar_cpp14_a5_2_4_violation]
        // Suppress “AUTOSAR_Cpp14_A4_7_1” rule finding: “An integer expression shall not lead to data loss..”
        // Rationale: Overflow / underflow is not possible here
        // coverity[autosar_cpp14_a4_7_1_violation]
        const auto event = reinterpret_cast<const struct inotify_event*>(it.current());
        // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
        events.emplace_back(*event);

        // coverity[autosar_cpp14_m5_0_8_violation]
        // coverity[autosar_cpp14_m5_0_9_violation]
        // coverity[autosar_cpp14_a4_7_1_violation]
        std::advance(it, static_cast<ptrdiff_t>(sizeof(struct inotify_event) + event->len));
    }

    return events;
}

score::cpp::expected<NonBlockingFileDescriptor, Error>
InotifyInstanceImpl::InitializeInotify(Inotify& inotify, Fcntl& fcntl, const std::shared_ptr<Unistd>& unistd) noexcept
{
    const auto expected_inotify_file_descriptor = inotify.inotify_init();
    if (!(expected_inotify_file_descriptor.has_value()))
    {
        return score::cpp::make_unexpected(expected_inotify_file_descriptor.error());
    }
    auto inotify_file_descriptor = expected_inotify_file_descriptor.value();

    auto nonblocking_inotify_file_descriptor = NonBlockingFileDescriptor::Make(inotify_file_descriptor, fcntl, unistd);
    if (!(nonblocking_inotify_file_descriptor.has_value()))
    {
        return score::cpp::make_unexpected(nonblocking_inotify_file_descriptor.error());
    }

    return std::move(nonblocking_inotify_file_descriptor.value());
}

}  // namespace os
}  // namespace score
