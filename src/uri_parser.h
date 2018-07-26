#ifndef _URI_PARSER_H_
#define _URI_PARSER_H_
#include <string>
#include <boost/optional.hpp>
#include <iostream>

struct URIAuthority {
  boost::optional<std::string> userinfo;
  std::string host;
  boost::optional<std::string> port;

  bool operator==(const URIAuthority& other) const;
};

struct URI {
  std::string scheme;
  URIAuthority authority;
  std::string path;
  boost::optional<std::string> query;
  boost::optional<std::string> fragment;

  bool operator==(const URI& other) const;
};

std::ostream& operator<<(std::ostream& s, const URIAuthority& authority);
std::ostream& operator<<(std::ostream& s, const URI& uri);

boost::optional<URIAuthority> ParseURIAuthority(const std::string& authority);
boost::optional<URI> ParseURI(const std::string& uri);
#endif // _URI_PARSER_H_
