#include "Server.hpp"

namespace irc {

void Server::mode_user_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  std::string nick = client.get_nickname();
  if (message.size() == 2) {
    // 221 answer a query about clients's own modes
    queue_.push(
        std::make_pair(fd, numeric_reply_(221, fd, client.get_usermodes())));
    return;
  }
  // only the server operator can change modes, otherwise command silently
  // ignored
  if (!client.get_server_operator_status()) {
#if DEBUG
    std::cout << "you are not operator!";
#endif
    return;
  }
  std::string flags = message[2];
  if (message.size() > 3) {
    // 421 unknown command
    queue_.push(std::make_pair(fd, numeric_reply_(421, fd, message[3])));
  }
  bool sign = true;
  bool badflag = false;
  std::map<bool, std::string> map_sign_flag;  // store the changed values as +/-
                                              // to be printed out at the end
  std::vector<char> addedflags;
  std::vector<char> removedflags;
  // check for valid flags, adding them to the list or throwing errors
  for (size_t i = 0; i < flags.size(); ++i) {
    char current = flags.at(i);
    if (current == '+') {
      sign = true;
    } else if (current == '-') {
      sign = false;
    }
    // handle -o
    else if (current == 'o' && !sign && client.get_server_operator_status()) {
      client.set_server_operator_status(0);
      removedflags.push_back('o');
    }
    // ignore +o
    else if (current == 'o' && sign == true) {
      continue;
    }
    // handle +s
    else if (current == 's' && sign && !client.get_server_notices_status()) {
      client.set_server_notices_status(1);
      addedflags.push_back('s');
    }
    // handle -s
    else if (current == 's' && !sign && client.get_server_notices_status()) {
      client.set_server_notices_status(0);
      removedflags.push_back('s');
    }
    // setting the bool, it can be set more than once but error msg will only be
    // printed once
    else if (current != 's' && current != 'o' && current != '+' &&
             current != '-') {
      badflag = true;
    }
  }
  if (badflag) {
    // 501 err_ unknown mode flag
    queue_.push(std::make_pair(fd, numeric_reply_(501, fd, "")));
  }
  if (addedflags.empty() && removedflags.empty()) {
    return;
  }
  std::string flags_changed = "ft_irc: MODE " + nick;

  if (!removedflags.empty()) {
    flags_changed += " -";
    for (size_t i = 0; i < removedflags.size(); ++i) {
      flags_changed += removedflags.at(i);
    }
  }
  if (!addedflags.empty()) {
    flags_changed += " +";
    for (size_t i = 0; i < addedflags.size(); ++i) {
      flags_changed += addedflags.at(i);
    }
  }
  queue_.push(std::make_pair(fd, flags_changed));
}

void Server::mode_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  // this error is wrong
  if (message.size() < 2) {
    // Error 461: Not enough parameters
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd, "MODE")));
    return;
  }
  if (message[1].at(0) == '#') {
    // is channel mode
    if (!channels_.count(message[1])) {
      // 401 no such channel
      queue_.push(std::make_pair(fd, numeric_reply_(403, fd, message[1])));
      return;
    } else {
      if (message.size() < 3) {
        mode_print_flags_(fd, channels_[message[1]]);
        return;
      }
    }
    Channel &channel = channels_[message[1]];
    mode_channel_(fd, message, channel);
  } else {
    // is user mode
    std::string nick = client.get_nickname();
    if (!irc_stringissame(nick, message[1])) {
      if (!map_name_fd_.count(message[1])) {
        // 401 no such nickname
        queue_.push(std::make_pair(fd, numeric_reply_(401, fd, message[1])));
        return;
      } else {
        // 502 can't change mode for other users
        queue_.push(std::make_pair(fd, numeric_reply_(502, fd, "")));
        return;
      }
    }
    mode_user_(fd, message);
  }
}

void Server::mode_channel_(int fd, std::vector<std::string> &message,
                           Channel &channel) {
  Client &client = clients_[fd];
  if (!channel.get_operators().count(client.get_nickname())) {
    // check only for +b without arg flag
    check_plus_b_no_arg_flag_(fd, message, channel);
    return;
  }
  // For giving info at the end
  std::vector<char> added_modes, removed_modes;
  std::vector<std::string> added_mode_arguments, removed_mode_arguments;

  // For parsing the modestring and matching it with the arguments
  bool sign = true;
  std::string &modestring = message[2];
  std::vector<std::string>::iterator argument_iterator(&(message[3]));
  std::vector<std::string>::iterator end_iterator = message.end();

  for (size_t i = 0; i < modestring.size(); ++i) {
    char current = modestring.at(i);

    if (current == '+') {
      sign = true;
    } else if (current == '-') {
      sign = false;
    } else if (mode_functions_.find(current) != mode_functions_.end()) {
      // Execute the abomination of a function pointer in the map
      std::pair<size_t, std::string> ret = (this->*mode_functions_[current])(
          fd, channel, sign, argument_iterator, end_iterator);
      // If successful, add it to the vector to give information at the end
      // For loop especially for MODE -b, when removing several bans
      if (ret.first) {
        for (size_t i = 0; i < ret.first; ++i) {
          sign ? added_modes.push_back(current)
               : removed_modes.push_back(current);
        }
      }
      // If it was a successfull command that required an argument, the argument
      // is then added to the vector to give information at the end
      if (!ret.second.empty()) {
        sign ? added_mode_arguments.push_back(ret.second)
             : removed_mode_arguments.push_back(ret.second);
      }
    } else {
      // Error 472: is unknown mode char to me
      queue_.push(
          std::make_pair(fd, numeric_reply_(472, fd, std::string(1, current))));
    }
  }
  // If one or more commands were successful, send an info message to the
  // whole channel
  if (!added_modes.empty() || !removed_modes.empty()) {
    mode_channel_successmessage_(fd, channel, added_modes, removed_modes,
                                 added_mode_arguments, removed_mode_arguments);
  }
}

void Server::mode_channel_successmessage_(
    int fd, Channel &channel, std::vector<char> &added_modes,
    std::vector<char> &removed_modes,
    std::vector<std::string> &added_mode_arguments,
    std::vector<std::string> &removed_mode_arguments) {
  std::stringstream servermessage;
  servermessage << ":" << clients_[fd].get_nickname() << " MODE "
                << channel.get_channelname() << " ";

  if (!removed_modes.empty()) {
    servermessage << "-";
    for (size_t i = 0; i < removed_modes.size(); ++i)
      servermessage << removed_modes[i];
  }

  if (!added_modes.empty()) {
    servermessage << "+";
    for (size_t i = 0; i < added_modes.size(); ++i)
      servermessage << added_modes[i];
  }

  if (!removed_mode_arguments.empty()) {
    for (size_t i = 0; i < removed_mode_arguments.size(); ++i)
      servermessage << " " << removed_mode_arguments[i];
  }

  if (!added_mode_arguments.empty()) {
    for (size_t i = 0; i < added_mode_arguments.size(); ++i)
      servermessage << " " << added_mode_arguments[i];
  }

  send_message_to_channel_(channel, servermessage.str());
}

std::pair<size_t, std::string> Server::mode_channel_n_(
    int fd, Channel &channel, bool plus,
    std::vector<std::string>::iterator &arg,
    std::vector<std::string>::iterator &end) {
  (void)fd;
  (void)arg;
  (void)end;
  // if the mode is already set to that version then return silently
  if ((plus && channel.checkflag(C_OUTSIDE)) ||
      (!plus && !channel.checkflag(C_OUTSIDE))) {
    return std::make_pair(0, std::string());
  } else if (plus) {
    channel.setflag(C_OUTSIDE);
    return std::make_pair(1, std::string());
  } else {
    channel.clearflag(C_OUTSIDE);
    return std::make_pair(1, std::string());
  }
  return std::make_pair(0, std::string());
}

std::pair<size_t, std::string> Server::mode_channel_o_(
    int fd, Channel &channel, bool plus,
    std::vector<std::string>::iterator &arg,
    std::vector<std::string>::iterator &end) {
  if (arg == end) {
    // if no argument is given, ignore silently
    return std::make_pair(0, "");
  }

  std::string &name = *(arg++);
  if (!channel.is_user(name)) {
    // Error 401: No such nick
    queue_.push(std::make_pair(fd, numeric_reply_(401, fd, name)));
    return std::make_pair(0, "");
  }

  // +o
  if (plus) {
    if (channel.is_operator(name)) {
      // ignore silently
      return std::make_pair(0, "");
    }
    channel.add_operator(name);
    return std::make_pair(1, name);
  }
  // -o
  else {
    if (channel.is_operator(name)) {
      channel.remove_operator(name);
      return std::make_pair(1, name);
    }
    // ignore silently
    return std::make_pair(0, "");
  }
}

std::pair<size_t, std::string> Server::mode_channel_i_(
    int fd, Channel &channel, bool plus,
    std::vector<std::string>::iterator &arg,
    std::vector<std::string>::iterator &end) {
  (void)fd;
  (void)arg;
  (void)end;
  // if the mode is already set to that version then return silently
  if ((plus && channel.checkflag(C_INVITE)) ||
      (!plus && !channel.checkflag(C_INVITE))) {
    return std::make_pair(0, std::string());
  } else if (plus) {
    channel.setflag(C_INVITE);
    return std::make_pair(1, std::string());
  } else {
    channel.clearflag(C_INVITE);
    return std::make_pair(1, std::string());
  }
  return std::make_pair(0, std::string());
}

std::pair<size_t, std::string> Server::mode_channel_t_(
    int fd, Channel &channel, bool plus,
    std::vector<std::string>::iterator &arg,
    std::vector<std::string>::iterator &end) {
  // if the mode is already set to that version then return silently
  if ((plus && channel.checkflag(C_TOPIC)) ||
      (!plus && !channel.checkflag(C_TOPIC))) {
    return std::make_pair(0, std::string());
  } else if (plus) {
    channel.setflag(C_TOPIC);
    return std::make_pair(1, std::string());
  } else {
    channel.clearflag(C_TOPIC);
    return std::make_pair(1, std::string());
  }
  (void)fd;
  (void)end;
  (void)arg;
  return std::make_pair(0, std::string());
}

std::pair<size_t, std::string> Server::mode_channel_m_(
    int fd, Channel &channel, bool plus,
    std::vector<std::string>::iterator &arg,
    std::vector<std::string>::iterator &end) {
  if ((plus && channel.checkflag(C_MODERATED)) ||
      (!plus && !channel.checkflag(C_MODERATED))) {
    return std::make_pair(0, std::string());
  } else if (plus) {
    channel.setflag(C_MODERATED);
    return std::make_pair(1, std::string());
  } else {
    channel.clearflag(C_MODERATED);
    return std::make_pair(1, std::string());
  }
  (void)fd;
  (void)end;
  (void)arg;
  return std::make_pair(0, std::string());
}

std::pair<size_t, std::string> Server::mode_channel_l_(
    int fd, Channel &channel, bool plus,
    std::vector<std::string>::iterator &arg,
    std::vector<std::string>::iterator &end) {
  // if "-l"
  if (!plus) {
    if (channel.get_user_limit() == 0) {
      return std::make_pair(0, std::string());
    } else {
      channel.set_user_limit(0);
      return std::make_pair(1, std::string());
    }
  }
  // if "+l"
  else {
    if (arg == end) {
      // 461 not enough parameters
      queue_.push(std::make_pair(fd, numeric_reply_(461, fd, channel.get_channelname())));
      return std::make_pair(0, std::string());
    }
    std::string tmp_arg = (*arg);
    int newlimit;
    if (!is_valid_userlimit(tmp_arg)) {
      channel.set_user_limit(0);
      return std::make_pair(1, std::string("0"));
    } else {
      newlimit = atoi(tmp_arg.c_str());
    }
    arg++;
    if (newlimit <= 0) {
      return std::make_pair(0, std::string());
    }
    if ((size_t)newlimit == channel.get_user_limit()) {
      return std::make_pair(0, std::string());
    }
    channel.set_user_limit(newlimit);
    return std::make_pair(1, tmp_arg);
  }
  (void)fd;
}

std::pair<size_t, std::string> Server::mode_channel_b_(
    int fd, Channel &channel, bool plus,
    std::vector<std::string>::iterator &arg,
    std::vector<std::string>::iterator &end) {
  // No argument? Print a list of users
  if (arg == end) {
    mode_channel_b_list_(fd, channel);
    return std::make_pair(0, "");
  }
  // +b: ban a mask
  if (plus) {
    return mode_channel_b_add_banmask_(fd, channel, arg);
  }
  // -b: remove all banmasks that fit the argument
  else {
    return channel.remove_banmask(*(arg++));
  }
}

void Server::mode_channel_b_list_(int fd, const Channel &channel) {
  const std::vector<banmask> &list_banmasks = channel.get_banned_users();
  std::stringstream prefixstream;
  prefixstream << ":" << server_name_ << " 367 " << clients_[fd].get_nickname()
               << " " << channel.get_channelname() << " ";
  std::string prefix(prefixstream.str());

  // List all banmasks with RPL_BANLIST (367)
  for (size_t i = 0; i < list_banmasks.size(); ++i) {
    std::stringstream servermessage;
    const banmask &current = list_banmasks[i];
    servermessage << prefix << current.banned_nickname << "!"
                  << current.banned_username << "@" << current.banned_hostname
                  << " " << current.banned_by << " " << current.time_of_ban;
    queue_.push(std::make_pair(fd, servermessage.str()));
  }

  // End with RPL_ENDBANLIST (368)
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 368 " << clients_[fd].get_nickname()
                << " " << channel.get_channelname()
                << " :End of Channel Ban List";
  queue_.push(std::make_pair(fd, servermessage.str()));
}

std::pair<size_t, std::string> Server::mode_channel_b_add_banmask_(
    int fd, Channel &channel, std::vector<std::string>::iterator &arg) {
  std::string banmask_nickname;
  std::string banmask_username;
  std::string banmask_hostname;
  parse_banmask(*arg++, banmask_nickname, banmask_username, banmask_hostname);

  // Is the banmask already covered by the existing masks?
  const std::vector<banmask> &list_banmasks = channel.get_banned_users();
  for (size_t i = 0; i < list_banmasks.size(); ++i) {
    const banmask &current = list_banmasks[i];
    if (irc_wildcard_cmp(banmask_nickname.c_str(),
                         current.banned_nickname.c_str()) &&
        irc_wildcard_cmp(banmask_username.c_str(),
                         current.banned_username.c_str()) &&
        irc_wildcard_cmp(banmask_hostname.c_str(),
                         current.banned_hostname.c_str()))
      return std::make_pair(0, "");
  }
  channel.remove_banmask(banmask_nickname + "!" + banmask_username + "@" + banmask_hostname);
  channel.add_banmask(banmask_nickname, banmask_username, banmask_hostname,
                      clients_[fd].get_nickname());
  std::stringstream new_mask;
  new_mask << banmask_nickname << "!" << banmask_username << "@"
           << banmask_hostname;
  return std::make_pair(1, new_mask.str());
}

std::pair<size_t, std::string> Server::mode_channel_v_(
    int fd, Channel &channel, bool plus,
    std::vector<std::string>::iterator &arg,
    std::vector<std::string>::iterator &end) {
  if (arg == end) {
    // Error 461: Not enough parameters
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd, "MODE +/-v")));
    return std::make_pair(0, "");
  }
  std::string nickname = (*arg);
  arg++;
  // if the nickname is not valid
  if (!channel.is_user(nickname)) {
    queue_.push(std::make_pair(fd, numeric_reply_(401, fd, nickname)));
    return std::make_pair(0, std::string());
  }
  // if the user is not on the speaker list
  if (!channel.get_speakers().count(nickname)) {
    if (plus) {
      channel.add_speaker(nickname);
      return std::make_pair(1, nickname);
    } else {
      return std::make_pair(0, std::string());
    }
  }
  // if the user is on the speaker list
  else {
    if (!plus) {
      channel.remove_speaker(nickname);
      return std::make_pair(1, nickname);
    } else {
      return std::make_pair(0, std::string());
    }
  }
  return std::make_pair(0, std::string());
}

std::pair<size_t, std::string> Server::mode_channel_k_(
    int fd, Channel &channel, bool plus,
    std::vector<std::string>::iterator &arg,
    std::vector<std::string>::iterator &end) {
  if (arg == end) {
    // Error 461: Not enough parameters
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd, channel.get_channelname())));
    return std::make_pair(0, "");
  }
  std::string &key = *(arg++);
  // if +k
  if (plus) {
    // If password is already set, give an error
    if (!channel.get_channel_password().empty()) {
      // Error 467: Channel key already set
      queue_.push(std::make_pair(
          fd, numeric_reply_(467, fd, channel.get_channelname())));
      return std::make_pair(0, "");
    }
    if (!channel_key_is_valid(key)) {
      // Error 525: Key is not well-formed
      queue_.push(std::make_pair(
          fd, numeric_reply_(525, fd, channel.get_channelname())));
      return std::make_pair(0, "");
    }
    channel.set_channel_password(key);
    return std::make_pair(1, key);
  }
  // else -k
  else {
    if (key != channel.get_channel_password()) {
/*       // Error 467: Channel key already set <- strange error, but quakenet sends
      // this
      queue_.push(std::make_pair(
          fd, numeric_reply_(467, fd, channel.get_channelname()))); */
      return std::make_pair(0, "");
    }
    std::string emptypw;
    channel.set_channel_password(emptypw);
    return std::make_pair(1, "");
  }
}

void Server::check_plus_b_no_arg_flag_(int fd,
                                       std::vector<std::string> &message,
                                       Channel &channel) {
  bool sign = true;
  bool not_operator_msg = false;
  std::string &modestring = message[2];
  std::vector<std::string>::iterator arg(&(message[3]));
  std::vector<std::string>::iterator end = message.end();

  for (size_t i = 0; i < modestring.size(); ++i) {
    char current = modestring.at(i);

    if (current == '+') {
      sign = true;
    } else if (current == '-') {
      sign = false;
    } else if (current == 'i' || current == 't' || current == 'm' ||
               current == 'n' || (!sign && current == 'l')) {
      not_operator_msg = true;
    } else if (current == 'o' || current == 'b' || current == 'v' ||
               current == 'k' || (sign && current == 'l')) {
      if (arg != end) {
        arg++;
        not_operator_msg = true;
      } else if (sign && current == 'b') {
        mode_channel_b_list_(fd, channel);
      }
    } else {
      // Error 472: is unknown mode char to me
      queue_.push(
          std::make_pair(fd, numeric_reply_(472, fd, std::string(1, current))));
    }
  }
  if (not_operator_msg) {
    // 482 You're not channel operator
    queue_.push(
        std::make_pair(fd, numeric_reply_(482, fd, channel.get_channelname())));
  }
}

void Server::mode_print_flags_(int fd, Channel &channel) {
  std::string flags = "";
  std::vector<int> digit_args;
  std::vector<std::string> string_args;
  // get all active flags
  if (channel.checkflag(C_INVITE)) {
    flags += "i";
  }
  if (channel.checkflag(C_TOPIC)) {
    flags += "t";
  }
  if (channel.checkflag(C_OUTSIDE)) {
    flags += "n";
  }
  if (channel.checkflag(C_MODERATED)) {
    flags += "m";
  }
  if (channel.get_user_limit() > 0) {
    flags += "l";
    digit_args.push_back(channel.get_user_limit());
  }
  if (channel.get_channel_password() != "") {
    flags += "k";
    string_args.push_back(channel.get_channel_password());
  }
  std::stringstream output;
  output << channel.get_channelname();
  if (flags.size() > 0) {
    output << " +" << flags;
  }
  if (!digit_args.empty()) {
    output << " " << digit_args.at(0);
  }
  if (!string_args.empty()) {
    output << " " << string_args.at(0);
  }
  queue_.push(std::make_pair(fd, numeric_reply_(324, fd, output.str())));
  std::stringstream argument;
  argument << channel.get_channelname() << " " << channel.get_creationtime();
  queue_.push(std::make_pair(fd, numeric_reply_(329, fd, argument.str())));
}

}  // namespace irc