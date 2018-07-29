#include "uri_parser.h"
#include <boost/optional/optional_io.hpp>
#include <regex>

bool URIAuthority::operator==(const URIAuthority& other) const {
  return userinfo == other.userinfo && host == other.host && port == other.port;
}

bool URI::operator==(const URI& other) const {
  return scheme == other.scheme && authority == other.authority &&
         path == other.path && query == other.query &&
         fragment == other.fragment;
}

std::ostream& operator<<(std::ostream& s, const URIAuthority& authority) {
  return s << "{ " << authority.userinfo << ", " << authority.host << ", "
           << authority.port << " }";
}
std::ostream& operator<<(std::ostream& s, const URI& uri) {
  return s << "{ " << uri.scheme << ", " << uri.authority << ", " << uri.path
           << ", " << uri.query << ", " << uri.fragment << " }";
}

boost::optional<URIAuthority> ParseAuthority(const std::string& authority) {
  std::regex re("^(([^@]*)@)?(.*?)(:([0-9]*))?$");
  std::smatch match;
  if (!std::regex_match(authority, match, re)) {
    return boost::none;
  }
  URIAuthority result;
  if (match[2].matched) {
    result.userinfo = std::string(match[2].first, match[2].second);
  }
  result.host = std::string(match[3].first, match[3].second);
  if (match[5].matched) {
    result.port = std::string(match[5].first, match[5].second);
  }
  return std::move(result);
}

boost::optional<URI> ParseURI(const std::string& uri) {
  // Regular expression from RFC3986
  std::regex re("^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?");
  std::smatch match;
  if (!std::regex_match(uri, match, re)) {
    return boost::none;
  }
  URI result;
  result.scheme = std::string(match[2].first, match[2].second);
  auto opt_authority =
      ParseAuthority(std::string(match[4].first, match[4].second));
  if (!opt_authority) {
    return boost::none;
  }
  result.authority = *opt_authority;
  result.path = std::string(match[5].first, match[5].second);
  if (match[7].matched) {
    result.query = std::string(match[7].first, match[7].second);
  }
  if (match[9].matched) {
    result.fragment = std::string(match[9].first, match[9].second);
  }

  return std::move(result);
}

static boost::optional<uint8_t> DecodeHexChar(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return (c - 'a') + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return (c - 'A') + 10;
  }
  return boost::none;
}

std::string URIUnescape(const std::string& input) {
  std::string output;
  for (std::size_t i = 0; i < input.size(); i++) {
    if (input[i] == '%' && i + 2 < input.size()) {
      auto x = DecodeHexChar(input[i + 1]);
      auto y = DecodeHexChar(input[i + 2]);
      if (x && y) {
        output += (char)((*x << 4) + *y);
        i += 2;
        continue;
      }
    }
    output += input[i];
  }
  return output;
}
