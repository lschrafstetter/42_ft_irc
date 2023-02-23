#include "Client.hpp"
#include "Server.hpp"

namespace irc {

void Server::oper_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  if (message.size() < 3) {
    // Error 461: Not enough parameters
    queue_.push(
        std::make_pair(fd, numeric_reply_(461, fd, client.get_nickname())));
    return;
  }
  int user_fd = search_user_list_(message[1]);
  if (user_fd < 0) {
    queue_.push(
        // 444 User not logged in (cant find username)
        std::make_pair(fd, numeric_reply_(444, fd, message[1])));
    return;
  } else if (clients_[user_fd].get_server_operator_status() == 1) {
    // that user is already an operator
    return;
  }
  if (message[2] == operator_password_) {
    // 381 You are now an IRC operator
    clients_[user_fd].set_server_operator_status(1);
    queue_.push(std::make_pair(
        fd, numeric_reply_(381, fd, clients_[user_fd].get_username())));
  } else {
    // 464 password incorrect
    queue_.push(std::make_pair(fd, numeric_reply_(464, fd, message[2])));
  }
}

bool Server::valid_userflags_(int fd, std::string flags) {
  Client &client = clients_.[fd];
  std::string flagstoprint("irc: MODE " + client.get_nickname() + " ");
  bool ret = true;
  if (flags.size() < 2)
    return false;
  if (flags.at(0) != '+' && flags.at(0) != '-')
    return false;
  bool sign = true;
  if (flags.at(0) == '-')
    sign = false;
  flagstoprint += flags.at(0);
  for (size_t i = 1; i < flags.size(); ++i) {
    if (flags.at(i) == 'o' && sign == true)
      continue;
    else if (flags.at(i) == 'o' && sign == false) {
      client.set_server_operator_status(0);
      flagstoprint += "o";
    } else if (flags.at(i) == 's') {
      client.set_server_notices_status(sign);
      flagstoprint += "s";
    } else
      ret = false;
  }
  queue_.push(std::make_pair(fd, flagstoprint));
  return ret;
}

//<channel> +flags [<limit>] [<user>] [<ban mask>]
void Server::channel_mode_(int fd, std::vector<std::string> &message) {
  std::string &channel = message.at(1);
  Client &client = clients_[fd];
  if (!valid_channel_name(channel)) {
    // 403 no such channel
    queue_.push(std::make_pair(fd, numeric_reply_(403, fd, channel)));
    return;
  }
  if (message.size() == 2) {
    // 324 print out channel flags <channel> <mode> <mode params>
    std::string channelmodes =
        "irc: MODE " + channel + " " + channel.get_channelmodes();
    queue_.push(std::make_pair(fd, numeric_reply(324, channelmodes)));
    return;
  }
  if (!channel.get_operators().find(client)) {
    // err_chanoprivsneeded 482 <channel> :You're not channel operator
    queue_.push(std::make_pair(fd, numeric_reply_(482, fd, channel)));
    return;
  }
  handle_channel_flags_(fd, message);
  std::string channelmodes =
      "irc: MODE " + channel + " " + channel.get_channelmodes();
  queue_.push(std::make_pair(fd, numeric_reply(324, channelmodes)));
}

void Server::handle_channel_flags_(int fd, std::vector<std::string> &message) {
  std::string &flags = message.at(2);
  if (flags.at(0) != '+' && flags.at(0) != '-' || flags.size() < 2) {
    // 472 is unknown mode character to me
    queue_.push(std::make_pair(fd, numeric_reply_(472, fd, flags)));
    return;
  }
  if (message.size() > 4 || message.size() == 4 && flags.size > 2) {
    queue_.push(std::make_pair(
        fd, "irc: MODE too many flags/ arguments for this combination"));
    return;
  }
  bool sign = false;
  if (flags.at(0) == '+')
    sign = true;
  if ((flags.find("o") || flags.find("b")) && flags.size() > 4) {
    queue_.push(
        std::make_pair(fd, "irc: MODE too many flags for this combination"));
    return;
  }
  for (size_t i = 1; i < flags.size(); ++i) {
    if flags
      .at(i) == 'o' {
        // if the msg size is 4
        // check the nickname ******DO!***********
        // if +sign, add operator
        // if -sign, remove operator
        if (message.size() == 4) {
          if valid
            nickname {
              if (sign) {
                add_operator(message.at(3));
              } else { if (channel.is_operator(nickname???))
                  remove_operator(message.at(3));
              }
              else {
                for (size_t i = 0; i < get_operators().size(); ++i) {
                  std::string operator= get_operators().at(i) + " ";
                  queue._push(std::make_pair(fd, operator));
                }
              }
            }
          else {
            error message 401 wrong nickname
          }
        }
      }
    else if (flags.at(i) == 'i') {
      if (sign) {
        setflag(C_INVITE);
      } else {
        clearflag(C_INVITE);
      }
    } else if (flags.at(i) == 't') {
      if (sign) {
        setflag(C_TOPIC);
      } else {
        clearflag(C_TOPIC);
      }
    } else if (flags.at(i) == 'n') {
      if (sign) {
        setflag(C_OUTSIDE);
      } else {
        clearflag(C_OUTSIDE);
      }
    } else if (flags.at(i) == 'm') {
      if (sign) {
        setflag(C_MODERATED);
      } else {
        clearflag(C_MODERATED);
      }
    } else if (flags.at(i) == 'l') {
      if (message.size() == 4 && sign) {
        if (!sign || message.at(3).size() > 1 || !isdigit(message[3][0])) {
          queue_.push(fd, "Enter a user limit under 10");
        } else {
          channel.set_user_limit(atoi(message.at(3)));
        }
      } else if (message.size() == 3 && !sign) {
        channel.set_user_limit(10);
      } else {
        std::string limits =
            "irc: MODE " + channel + " user limit " + get_user_limit();
        queue_.push(std::pair(fd, limits));
      }
      // rpl_banlist 367 <channel> <banid> (A separate banlist is sent for each
      // banned user) rpl_endofbanlist 368 marks the end of the banlist
      // <channel> :End of channel ban list
    } else if (flags.at(i) == 'b') {
      if (message.size() == 4)
        // set a ban limit if its a valid user
        else
      // print the ban list
    } else if (flags.at(i) == 'v') {
      if (message.size() == 4)
        // set/ remove talking privileges
        else
      // print a list of the privileged
    } else if (flags.at(i) == 'k')
      // set a channel password
      // err_keyset 467 "<channel> :Channel key already set"

      else
    // print an error message char not found
  }
  // mode +b without nickname returns the following:
  // rpl_banlist 367 <channel> <banid> (A separate banlist is sent for each
  // banned user) rpl_endofbanlist 368 marks the end of the banlist <channel>
  // :End of channel ban list err_keyset 467 "<channel> :Channel key already
  // set" always sent at the end: rpl_channelmodeis 324 <channel> <mode> <mode
  // params>
}

//<channel> +flags [<limit>] [<user>] [<ban mask>]
// o - give/take channel operator privileges <user>
// i - invite only
// t - topic settable by chanop only
// n - no messages to channel from clients on the outside
// m - moderated channel
// l - set the user limit to channel <limit>
// b - set a ban mask to keep users out <ban mask>
// v - give/take the ability to speak on a moderated channel <user>
// k -set a channel key(password) <password>

void Server::mode_user_(int fd, std::vector<std::string> &message) {
  std::string nick = client.get_nickname();
  // server command
  // check for correct syntax
  if (!irc_stringissame(nick, message[1])) {
    if (search_nick_list_(message[1]) == 0) {
      // 401 no such nickname
      queue_.push(std::make_pair(fd, numeric_reply_(401, fd, message[2])));
      return;
    } else {
      // 502 can't change mode for other users
      queue_.push(std::make_pair(fd, numeric_reply_(502, fd, nick)));
      return;
    }
  }
  if (message.size() == 2) {
    // 221 answer a query about clients's own modes
    queue_.push(
        std::make_pair(fd, numeric_reply_(221, fd, client.get_usermodes_())));
    return;
  }
  // only the server operator can change modes, otherwise command silently
  // ignored
  if (!client.get_server_operator_status)
    return;
  // check for valid flags
  if (valid_userflags_(fd, message[2]) == false) {
    // 501 err_ unknownmode
    queue_.push(std::make_pair(fd, numeric_reply_(501, fd, message[2])));
  }
}

void Server::mode_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  // this error is wrong
  if (message.size() < 2) {
    // Error 461: Not enough parameters
    queue_.push(
        std::make_pair(fd, numeric_reply_(461, fd, client.get_nickname())));
    return;
  }
  if (message[1].at(0) == '#') {
    //is channeln mode
    if (!channels_.find(message[1])) {
      // 401 no such channel
      queue_.push(std::make_pair(fd, numeric_reply_(403, fd, message[1])));
      return;
    } else {
      mode_channel_(fd, message);
    }
  } else {
    //is user mode
    std::string nick = client.get_nickname();
    if (!irc_stringissame(nick, message[1])) {
      if (search_nick_list_(message[1]) == 0) {
        // 401 no such nickname
       queue_.push(std::make_pair(fd, numeric_reply_(401, fd, message[2])));
        return;
      } else {
      // 502 can't change mode for other users
        queue_.push(std::make_pair(fd, numeric_reply_(502, fd, nick)));
        return;
      }
    }
    mode_user_(fd, message);
  }
}

void Server::kill_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  if (message.size() < 3) {
    // Error 461: Not enough parameters
    queue_.push(
        std::make_pair(fd, numeric_reply_(461, fd, client.get_nickname())));
    return;
  }
  if (!client.get_server_operator_status()) {
    // 481,"Permission Denied- You're not an IRC operator"
    queue_.push(
        std::make_pair(fd, numeric_reply_(481, fd, client.get_nickname())));
    return;
  }
  if (!search_nick_list_(message.at(1))) {
    // 401 no such nickname
    queue_.push(std::make_pair(fd, numeric_reply_(401, fd, message[1])));
    return;
  }
  int victimfd = map_name_fd_[message[1]];
  // queue_.push(std::make_pair(victimfd, numeric_reply_(361, "")));
  std::vector<std::string> quitmessage(1, "QUIT");
  quitmessage.push_back("Killed(" + client.get_nickname() + "(" + message[2] +
                        "))");
  quit_(victimfd, quitmessage);
}

} // namespace irc