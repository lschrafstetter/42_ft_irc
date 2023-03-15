#include "Server.hpp"

namespace irc {

/**
 * @brief Sends a welcome message (replies 001 - 005) to a client after
 * successfull registration
 *
 * @param fd the client's file descriptor
 */
void Server::welcome_(int fd) {
  std::string clientname = clients_[fd].get_nickname();

  // 001 RPL_WELCOME
  {
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 001 " << clientname
                  << " :Welcome to ircserv, " << clientname;
    queue_.push(std::make_pair(fd, servermessage.str()));
  }

  // 002 RPL_YOURHOST
  {
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 002 " << clientname
                  << " :Your host is " << server_name_
                  << ", running on version 1.0";
    queue_.push(std::make_pair(fd, servermessage.str()));
  }

  // 003 RPL_CREATED
  {
    std::stringstream servermessage;
    char timebuffer[80];
    std::memset(&timebuffer, 0, sizeof(timebuffer));
    std::strftime(timebuffer, sizeof(timebuffer), "%a %b %d %Y at %H:%M:%S %Z",
                  std::localtime(&creation_time_));
    servermessage << ":" << server_name_ << " 003 " << clientname
                  << " :This server was created " << timebuffer;
    queue_.push(std::make_pair(fd, servermessage.str()));
  }

  // 004 RPL_MYINFO
  {
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 004 " << clientname
                  << " ircserv 1.0 so oitnmlbvk olbvk";
    queue_.push(std::make_pair(fd, servermessage.str()));
  }

  // 005 RPL_ISUPPORT
  {
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 005 " << clientname
                  << " MAXCHANNELS=10 NICKLEN=9 CHANMODES=b,k,l,imnt :are "
                     "supported by this server";
    queue_.push(std::make_pair(fd, servermessage.str()));
  }

  // Empty helper vector
  std::vector<std::string> vec;

  // LUSER message
  lusers_(fd, vec);

  // MOTD message
  motd_(fd, vec);
}

/**
 * @brief Sends information about the server to the client. Can be called by the
 * welcome_ function after registering or by a LUSERS command
 *
 * @param fd the clients file descriptor
 * @param message doesn't matter. message[0] is "LUSERS" if the function is
 * called by a LUSERS command called after parsing
 */
void Server::lusers_(int fd, std::vector<std::string> &message) {
  (void)message;

  // 251 RPL_LUSERCLIENT (mandatory)
  // 252 RPL_LUSEROP (only if non-zero)
  // 253 RPL_LUSERUNKNOWN (only if non-zero)
  lusers_client_op_unknown_(fd);

  // 254 RPL_LUSERCHANNELS (only if non-zero)
  lusers_channels_(fd);

  // 255 RPL_LUSERME (mandatory)
  lusers_me_(fd);
}

void Server::lusers_client_op_unknown_(int fd) {
  int n_users_invis = 0;
  int n_users_non_invis = 0;
  int n_operators = 0;
  int n_unauthorized = 0;

  std::map<int, Client>::iterator it = clients_.begin();
  std::map<int, Client>::iterator end = clients_.end();

  while (it != end) {
    /* if ((*it).second.is_invisible())
      ++n_users_invis;
    else */
    ++n_users_non_invis;

    // if ((*it).second.is_operator()) ++n_operators;

    if (!(*it).second.is_authorized()) ++n_unauthorized;

    ++it;
  }

  // 251 RPL_LUSERCLIENT (mandatory)
  {
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 251 "
                  << clients_[fd].get_nickname() << " :There are "
                  << n_users_non_invis << " users and " << n_users_invis
                  << " invisible on 1 servers";
    queue_.push(std::make_pair(fd, servermessage.str()));
  }
  // 252 RPL_LUSEROP (only if non-zero)
  if (n_operators) {
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 252 "
                  << clients_[fd].get_nickname() << " " << n_operators
                  << " :operator(s) online";
    queue_.push(std::make_pair(fd, servermessage.str()));
  }

  // 253 RPL_LUSERUNKNOWN (only if non-zero)
  if (n_unauthorized) {
    std::stringstream servermessage;
    servermessage.str(std::string());
    servermessage << ":" << server_name_ << " 253 "
                  << clients_[fd].get_nickname() << " " << n_unauthorized
                  << " :unknown connection(s)";
    queue_.push(std::make_pair(fd, servermessage.str()));
  }
}

void Server::lusers_channels_(int fd) {
  int n_channels = channels_.size();

  if (n_channels) {
    std::stringstream servermessage;
    servermessage << ":" << server_name_ << " 254 "
                  << clients_[fd].get_nickname() << n_channels
                  << " :channels formed";
    queue_.push(std::make_pair(fd, servermessage.str()));
  }
}

void Server::lusers_me_(int fd) {
  std::stringstream servermessage;
  servermessage << ":" << server_name_ << " 255 " << clients_[fd].get_nickname()
                << " :I have " << clients_.size() << " clients and 1 servers";
  queue_.push(std::make_pair(fd, servermessage.str()));
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
    queue_.push(std::make_pair(fd, numeric_reply_(402, fd, message[1])));
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
#if DEBUG
    std::cout << "Trying to open file" << std::endl;
#endif
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

}  // namespace irc