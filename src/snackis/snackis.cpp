#include "snackis.hpp"

#include <sodium.h>
#include <sstream>
#include "snackis/core/stream.hpp"
#include "snackis/crypt/error.hpp"

namespace snackis {
  const int VERSION[3] = {0, 6, 3};
  const int64_t DB_REV = 1;
  const int64_t PROTO_REV = 1;
  
  void init() {
    if (sodium_init() == -1) {
      ERROR(Crypt, "Failed initializing libsodium");
    }
  }

  str version_str() {
    Stream out;

    for (int i = 0; i < 3; i++) {
      if (i) { out << "."; }
      out << VERSION[i];
    }
    
    return out.str();
  }
}
