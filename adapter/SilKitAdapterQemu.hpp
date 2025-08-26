// SPDX-FileCopyrightText: Copyright 2025 Vector Informatik GmbH
// SPDX-License-Identifier: MIT

#pragma once

#include <string>

namespace adapters {

  /// <summary>
  /// string containing the argument preceding ethernet adaptation information.
  /// </summary>
  extern const std::string ethArg;

  /// <summary>
  /// string containing the argument preceding chardev adaptation information.
  /// </summary>
  extern const std::string chardevArg;

  /// <summary>
  /// string containing the argument preceding ethernet adaptation and unix domain socket information.
  /// </summary>
  extern const std::string unixEthArg;

  /// <summary>
  /// string containing the value in case there is no participant name argument.
  /// </summary>
  extern const std::string defaultParticipantName;
}