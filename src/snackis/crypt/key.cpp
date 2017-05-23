#include "snackis/crypt/error.hpp"
#include "snackis/crypt/key.hpp"

namespace snackis {
namespace crypt {
  Key::Key() {
    crypto_box_keypair(pub.data, data);
  }

  std::vector<unsigned char> encrypt(const Key &key, const PubKey &pub_key,
				     const unsigned char *in,
				     size_t len) {
    std::vector<unsigned char> out;
    out.resize(crypto_box_NONCEBYTES+crypto_box_MACBYTES+len, 0);
    randombytes_buf(&out[0], crypto_box_NONCEBYTES);
    
    if (crypto_box_easy(&out[crypto_box_NONCEBYTES],
			in, len,
			&out[0],
			pub_key.data, key.data) != 0) {
      ERROR(Crypt, "failed encrypting data");
    }

    return out;
  }

  std::vector<unsigned char> decrypt(const Key &key, const PubKey &pub_key,
				     const unsigned char *in,
				     size_t len) {
    std::vector<unsigned char> out;
    out.resize(len-crypto_box_NONCEBYTES-crypto_box_MACBYTES, 0);
    
    if (crypto_box_open_easy(&out[0],
			     &in[crypto_box_NONCEBYTES], len-crypto_box_NONCEBYTES,
			     &in[0],
			     pub_key.data, key.data) != 0) {
      ERROR(Crypt, "failed decrypting data");
    }

    return out;
  }

}}
