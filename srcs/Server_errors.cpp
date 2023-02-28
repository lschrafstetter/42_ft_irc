#include "Server.hpp"

namespace irc {

std::string Server::numeric_reply_(int error_number, int fd_client,
                                   std::string argument) {
  std::ostringstream ss;
  ss << ":" << server_name_ << " " << error_number << " "
     << clients_[fd_client].get_nickname();

  if (argument.size()) ss << " ";

  ss << argument << " " << error_codes_[error_number];
  return ss.str();
}

void Server::init_error_codes_() {
  error_codes_.insert(
      std::make_pair<int, std::string>(221, ""));  //<user mode string>
  error_codes_.insert(std::make_pair<int, std::string>(
      324, ""));  //<channel> <mode> <mode params>
  error_codes_.insert(std::make_pair<int, std::string>(
      329, ""));  // rpl creationtime -- maybe these digits are the time it was
                  // created?
  error_codes_.insert(std::make_pair<int, std::string>(
      341, ""));  //<nick> <channel> <channel invite>
  error_codes_.insert(std::make_pair<int, std::string>(
      381, ":You are now an IRC operator"));  //:
  error_codes_.insert(std::make_pair<int, std::string>(
      401, ":No such nick"));  //<nickname> : No such nick/ channel
  error_codes_.insert(std::make_pair<int, std::string>(
      402, ":No such server"));  //<server name> :No such server
  error_codes_.insert(std::make_pair<int, std::string>(
      403, ":No such channel"));  // <channel name> :No such channel
  error_codes_.insert(std::make_pair<int, std::string>(
      404,
      ":Cannot send to channel"));  //<channel name> :Cannot send to channel
  error_codes_.insert(std::make_pair<int, std::string>(
      405, ":You have joined too many channels"));  // <channel name> :You have
                                                    // joined too many channels
  error_codes_.insert(std::make_pair<int, std::string>(
      411, ":No recipient given"));  //:No recipient given (<command>)
  error_codes_.insert(std::make_pair<int, std::string>(
      412, ":No text to send"));  //:No text to send
  error_codes_.insert(std::make_pair<int, std::string>(
      421, ":Unknown command"));  //<command> :Unknown command
  error_codes_.insert(std::make_pair<int, std::string>(
      422, ":MOTD File is missing"));  //:MOTD File is missing
  error_codes_.insert(std::make_pair<int, std::string>(
      431, ":No nickame given"));  // :No nickname given
  error_codes_.insert(std::make_pair<int, std::string>(
      432, ":Erroneous nickname"));  //<nick> :Erroneous nickname
  error_codes_.insert(std::make_pair<int, std::string>(
      433,
      ":Nickname is already in use"));  //<nick> :Nickname is already in use
  error_codes_.insert(std::make_pair<int, std::string>(
      441, ":They aren't on that channel"));  //<nick> <channel> :They aren't on
                                              // that channel
  error_codes_.insert(std::make_pair<int, std::string>(
      442,
      ":You're not on that channel"));  //<channel> :You're not on that channel
  error_codes_.insert(std::make_pair<int, std::string>(
      443,
      ":is already on channel"));  //<user> <channel> :is already on channel
  error_codes_.insert(std::make_pair<int, std::string>(
      444, ":User not logged in"));  //<user> :User not logged in
  error_codes_.insert(std::make_pair<int, std::string>(
      451, ":You have not registered"));  //:You have not registered
  error_codes_.insert(std::make_pair<int, std::string>(
      461, ":Not enough parameters"));  //<command> :Not enough parameters
  error_codes_.insert(std::make_pair<int, std::string>(
      462, ":You may not reregister"));  //:You may not reregister
  error_codes_.insert(std::make_pair<int, std::string>(
      464, ":Password incorrect"));  //:Password incorrect
  error_codes_.insert(std::make_pair<int, std::string>(
      467, ":Channel key already set"));  //<channel> :Channel key already set
  error_codes_.insert(std::make_pair<int, std::string>(
      476, ":Bad channel mask"));  // no info; optional message
  error_codes_.insert(std::make_pair<int, std::string>(
      471, ":Cannot join channel (+l)"));  //<channel> :Cannot join channel (+1)
  error_codes_.insert(std::make_pair<int, std::string>(
      472,
      ":is unknown mode char to me"));  //<char> :is unknown mode char to me
  error_codes_.insert(std::make_pair<int, std::string>(
      473,
      ":Cannot join channel (+i)"));  // <channel> :Cannot join channel (+i)
  error_codes_.insert(std::make_pair<int, std::string>(
      474, ":Cannot join channel (+b)"));  //<channel> :Cannot join channel (+b)
  error_codes_.insert(std::make_pair<int, std::string>(
      475, ":Cannot join channel (+k)"));  //<channel> :Cannot join channel (+k)
  error_codes_.insert(std::make_pair<int, std::string>(
      476, "Bad Channel Mask"));  // optional message
  error_codes_.insert(std::make_pair<int, std::string>(
      481,
      ":Permission Denied- You're not an IRC operator"));  //:Permission Denied-
                                                           // You're not an IRC
                                                           // operator
  error_codes_.insert(std::make_pair<int, std::string>(
      482, ":You're not channel operator"));  //<channel> :You're not channel
                                              // operator
  error_codes_.insert(std::make_pair<int, std::string>(
      501, ":Unknown MODE flag"));  //:Unkown MODE flag
  error_codes_.insert(std::make_pair<int, std::string>(
      502, ":Can't change mode for other users"));  //:Can't change mode for
                                                    // other users
  error_codes_.insert(
      std::make_pair<int, std::string>(525, ":Key is not well-formed"));
}
}  // namespace irc