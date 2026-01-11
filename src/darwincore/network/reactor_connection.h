//
// DarwinCore Network Module
// Reactor Internal Connection Structure
//
// Description:
//   Internal connection structure used only within Reactor.
//   This structure is NOT exposed to users or Worker threads.
//
// Design Rule:
//   - fd is an IO resource, owned exclusively by Reactor
//   - connection_id is a business resource, exposed to upper layers
//   - NEVER pass this structure across threads
//   - Worker threads only see ConnectionInformation (no fd)
//
// Author: DarwinCore Network Team
// Date: 2026

#ifndef DARWINCORE_NETWORK_REACTOR_CONNECTION_H
#define DARWINCORE_NETWORK_REACTOR_CONNECTION_H

#include <cstdint>
#include <cstring>  // For memset
#include <sys/socket.h>

namespace darwincore {
namespace network {

/**
 * @brief Internal connection structure for Reactor
 *
 * This structure represents a network connection inside the Reactor.
 * It contains the file descriptor (fd), peer address, and connection ID.
 *
 * IMPORTANT:
 * - This is an INTERNAL structure, do NOT expose to users
 * - NEVER share this with Worker threads (fd must stay in Reactor)
 * - ConnectionInformation (without fd) is the external interface
 */
struct ReactorConnection {
  /// File descriptor for the socket (owned by Reactor thread only)
  int file_descriptor;

  /// Peer address information
  sockaddr_storage peer;

  /// Unique connection identifier (used by upper layers)
  uint64_t connection_id;

  /// Default constructor - initialize with invalid values
  ReactorConnection()
      : file_descriptor(-1), connection_id(0) {
    memset(&peer, 0, sizeof(peer));
  }

  /// Constructor with parameters
  ReactorConnection(int fd, const sockaddr_storage& p, uint64_t id)
      : file_descriptor(fd), peer(p), connection_id(id) {}
};

}  // namespace network
}  // namespace darwincore

#endif  // DARWINCORE_NETWORK_REACTOR_CONNECTION_H
