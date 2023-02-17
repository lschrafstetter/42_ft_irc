#include "Client.hpp"
#include "Server.hpp"

namespace irc {

// called when the user enters PASS
void Server::authenticate_password_(int fd, std::vector<std::string> &message) {
  Client &client = clients_[fd];
  if (client.is_fully_authorized() == 1) {
    std::cout << "Password already authenticated\n";
    return;
  }
  if (message.size() == 2 && message[1] == password_) {
    std::cout << "Password authenticated; access permitted\n";
    clients_[fd].set_auth_status(PASS_AUTH);
  } else {
    std::cout << "Password incorrect; access denied\n";
  }
}

bool Server::search_nick_list(std::string nick) {
  std::string error_str;
  std::map<int, Client>::iterator it;
  for (it = clients_.begin(); it != clients_.end(); ++it) {
    if (nick == (*it).second.get_nickname()) {
      return 1;
    }
  }
  return 0;
}

void Server::set_username_(int fd, std::vector<std::string> &message) {
  // Work in progress
  // std::string error_msg;
  if (clients_[fd].get_auth_status(USER_AUTH)) {
    // error_msg = ":irc 462" + clients_[fd].get_nickname() + " :You may not
    // reregister";
    queue_.push(std::make_pair(fd, numeric_reply_(462, fd)));
    return;
  }
  if (message.size() < 4) {
    // error_msg = ":irc 461 " + clients_[fd].get_username() + " :Not enough
    // parameters";
    queue_.push(std::make_pair(fd, numeric_reply_(461, fd)));
    return;
  }
  // check for a valid username
  //: server 468 nick :Your username is invalid.
  //: server 468 nick :Connect with your real username, in lowercase.
  //: server 468 nick :If your mail address were foo@bar.com, your username
  //: would be foo.
  // return ;
  clients_[fd].set_username(message[1]);
  clients_[fd].set_auth_status(USER_AUTH);
}

void Server::set_nickname_(int fd, std::vector<std::string> &message) {
  std::string error_str;
  if (search_nick_list(message[1])) {
    error_str =
        ":irc " + clients_[fd].get_nickname() + " :Nickname is already in use.";
    queue_.push(std::make_pair(fd, error_str));
    return;
  }
  /* if (nickname_exists) {
    queue_.push(std::make_pair(fd, "ERR_NICKCOLLISION"));
    return;
  } */
  // Check list of nicknames!
  // Work in progress
  clients_[fd].set_nickname(message[1]);
  clients_[fd].set_auth_status(NICK_AUTH);
}

void Server::pong_(int fd, std::vector<std::string> &message) {
  if (message.size() != 2) return;

  Client &client = clients_[fd];
  if (client.get_expected_ping_response() == message[1]) {
    client.set_pingstatus(true);
    // client.set_auth_status(PONGFLAG)
  }

  // additionally check for authentication status??
}

/**
 * @brief sends the message of the day to a client, consisting of a start, a
 * message and and end message. The message is contained in a separate file
 * called "motd.txt"
 *
 * @param fd the client's file descriptor
 * @param message the command which was parsed, not the message itself.
 */
void Server::motd_(int fd, std::vector<std::string> &message) {
  if (message.size() > 1 && message[1].compare(server_name_) != 0) {
    // Error 402: No such server
    queue_.push(std::make_pair(fd, numeric_reply_(402, fd, " " + message[1])));
    return;
  }
  // RPL_MOTDSTART (375)
  motd_start_(fd);
  // RPL_MOTD (372)

  motd_message_(fd);
  // RPL_ENDOFMOTD (376)

  motd_end_(fd);
}

void Server::motd_start_(int fd) {
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 375 " << clients_[fd].get_nickname()
                << " :- " << server_name_ << " Message of the day - ";
  queue_.push(std::make_pair(fd, servermessage.str()));
}

void Server::motd_message_(int fd) {
  std::ifstream infile;
  try {
    std::cout << "Trying to open file" << std::endl;
    infile.open("ressources/motd.txt",
                std::ifstream::in | std::ifstream::binary);
    if (infile.fail()) throw std::exception();
  } catch (std::exception &e) {
    // Error 422: MOTD File is missing
    queue_.push(std::make_pair(fd, numeric_reply_(422, fd, "")));
    return;
  }

  std::string line;
  std::string clientname = clients_[fd].get_nickname();
  while (infile.good()) {
    std::getline(infile, line);
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 372 " << clientname << " :"
                  << line;
    queue_.push(std::make_pair(fd, servermessage.str()));
  }
  infile.close();
}

void Server::motd_end_(int fd) {
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 376 " << clients_[fd].get_nickname()
                << " :End of /MOTD command.";
  queue_.push(std::make_pair(fd, servermessage.str()));
}

void Server::join_(int fd, std::vector<std::string> &message) {
  #if DEBUG
    std::cout << "Join function called" << std::endl;
    std::cout << "FD: " << fd << std::endl;
    for (size_t i = 0; i < message.size(); ++i)
      std::cout << "Message " << i << ": " << message[i] << std::endl;
  #endif

  if (!message[1])
    #if DEBUG
        std::cout << "Error 461 :Not enough parameters" << std::endl;
    #endif
    // Error 461 :Not enough parameters

  std::vector<std::string>  channel_name_ = split_string(message[1], ';');
  std::vector<std::string>  channel_key_  = split_string(message[2], ';');
  if (channels_.find(channel_name_[0]) != channels_.end()) {
    Channel temp = (channels_.find(channel_name_[0]))->second;                             // check if user is already in channel?
    if (temp.checkflag(C_INVITE) && 1 /* !(clients_[fd].is_invited(channel_name_[0])) */)
      #if DEBUG
        std::cout << "Error 473 :Cannot join channel (+i)" << std::endl;
      #endif
      // Error 473 :Cannot join channel (+i)
    else if (temp.is_banned(clients_[fd].get_nickname()))
      #if DEBUG
        std::cout << "Error 473 :Cannot join channel (+b)" << std::endl;
      #endif
      // Error 474 :Cannot join channel (+b)
    else if (temp.get_channel_password() != "" && !irc_stringissame(temp.get_channel_password(), channel_key_[0]))
      #if DEBUG
        std::cout << "Error 473 :Cannot join channel (+k)" << std::endl;
      #endif
      // Error 475 :Cannot join channel (+k)
    else if (temp.get_users().size() >= temp.get_user_limit())
      #if DEBUG
        std::cout << "Error 473 :Cannot join channel (+l)" << std::endl;
      #endif
      // Error 471 :Cannot join channel (+l)

    // else if (clients_[fd].get_channels_list().size() >= max_channel_size)
      // Error 405 :You have joined too many channels

    //  adding clients by user or nick name?
  }
  else
    #if DEBUG
        std::cout << "Error 403 :No such channel" << std::endl;
    #endif
    // Error 403 :No such channel
}

void Server::remove_channel_(int fd, std::vector<std::string> &message) {
  // Check validity of message (size, parameters, etc...)
  clients_[fd].remove_channel(message[1]);
}

void Server::init_error_codes_() {
  error_codes_.insert(
      std::make_pair<int, std::string>(462, "You may not reregister"));
  error_codes_.insert(
      std::make_pair<int, std::string>(461, "Not enough parameters"));
}

std::string Server::numeric_reply_(int numeric, int fd) {
  std::ostringstream ss;
  ss << numeric;
  return (":" + server_name_ + " " + ss.str() + " " +
          clients_[fd].get_nickname() + " :" + error_codes_[numeric]);
}

/*
void Server::try_create_operator_(int fd, std::vector<std::string> &message) {
        if (client.get_server_operator_status() == 1) {
                return ;
        }
        if (password == operator_password_) {
                client.set_server_operator_status(1);
                std::cout <<"Password correct, " <<client.get_nickname() <<" now
has operator status\n";
        }
        else {
                std::cout <<"Password incorrect, operator status cannot be
given\n";
        }
}

void Server::remove_operator_(int fd, std::vector<std::string> &message) {
        if (client.get_server_operator_status() == 0) {
                return ;
        }
        if (password == operator_password_) {
                client.set_server_operator_status(0);
                std::cout <<"Operator status removed\n";
        }
        else {
                std::cout <<"Password incorrect, " <<client.get_nickname() <<"
still is an operator\n";
        }
} */

}  // namespace irc