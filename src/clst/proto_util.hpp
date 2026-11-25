#ifndef CLST_PROTO_UTIL_HPP
#define CLST_PROTO_UTIL_HPP

#include <google/protobuf/message.h>
#include <clst/varmap.hpp>

namespace clst {

// Insert protobuf messages into a VarMap.
// Maps protobuf data types to appropriate VarMap types.
// Nested protobuf messages are inserted recursively.
void insert_metadata(VarMap& map, const google::protobuf::Message& message);

} // namespace clst

#endif /* CLST_PROTO_UTIL_HPP */
