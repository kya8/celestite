#ifndef SLATE_PROTO_UTIL_HPP
#define SLATE_PROTO_UTIL_HPP

#include <google/protobuf/message.h>
#include <slate/varmap.hpp>

namespace slate {

// Insert protobuf messages into a VarMap.
// Maps protobuf data types to appropriate VarMap types.
// Nested protobuf messages are inserted recursively.
void insert_metadata(VarMap& map, const google::protobuf::Message& message);

} // namespace slate

#endif /* SLATE_PROTO_UTIL_HPP */
