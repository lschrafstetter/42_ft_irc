#include "Chatbot.hpp"

namespace irc {

bool running = true;

static void signalhandler(int signal) {
  (void)signal;
#if DEBUG
  std::cout << "Signalcode: " << signal << std::endl;
#endif
  running = false;
}

void Chatbot::run(const std::string &password) {
  (void)password;
  signal(SIGTSTP, signalhandler);
  epoll_init_();
  infinite_loop_(password);
}

void Chatbot::epoll_init_() {
  if ((fd_epoll_ = epoll_create(1)) < 0)
    throw std::runtime_error("Couldn't create epoll instance");

  struct epoll_event event;
  std::memset(&event, 0, sizeof(event));
  event.events = EPOLLIN;
  event.data.fd = fd_socket_;

  if (epoll_ctl(fd_epoll_, EPOLL_CTL_ADD, fd_socket_, &event) < 0)
    throw std::runtime_error("Couldn't add socket_fd to epoll watchlist");
}

void Chatbot::infinite_loop_(const std::string &password) {
  struct epoll_event postbox[1];
  char readbuffer[BUFFERSIZE];
  std::string messagebuffer;
  std::vector<std::string> message;

  std::memset(readbuffer, 0, BUFFERSIZE);
  while (running) {
    if (epoll_wait(fd_epoll_, postbox, 1, 1000) > 0) {
      if (read(fd_socket_, readbuffer, BUFFERSIZE) < 1) break;
      messagebuffer += readbuffer;
#if DEBUG
      std::cout << "Read: " << messagebuffer << std::endl;
#endif
      std::memset(readbuffer, 0, BUFFERSIZE);

      message = get_next_message_(messagebuffer);
      while (!message.empty()) {
        process_message_(message);
        message = get_next_message_(messagebuffer);
      }
    }
    if (!authenticated_ && (time(NULL) - last_auth_try_ > 10)) {
      send_authentication_request_(password);
    }
    while (!queue_.empty()) {
      send_message_(queue_.front());
      queue_.pop();
    }
  }
}

void Chatbot::send_message_(const std::string &message) {
  write(fd_socket_, message.c_str(), message.length());
  write(fd_socket_, "\r\n", 2);
}

std::vector<std::string> Chatbot::get_next_message_(std::string &buffer) {
  std::vector<std::string> ret;
  size_t end_of_message = buffer.find("\r\n");

  if (end_of_message == std::string::npos) return ret;

  std::string message = buffer.substr(0, end_of_message);
  buffer.erase(0, end_of_message + 2);

  // Looks for a prefix and discards it
  size_t pos;

  while ((pos = message.find(" ")) != std::string::npos) {
    if (pos > 0) ret.push_back(message.substr(0, pos));
    message.erase(0, pos + 1);
    if (message.size() && message.at(0) == ':') {
      ret.push_back(message.substr(1, message.size() - 1));
#if DEBUG
      std::cout << "Parsed next message:";
      for (size_t i = 0; i < ret.size(); ++i) {
        std::cout << " " << ret[i];
      }
      std::cout << std::endl;
#endif
      return ret;
    }
  }

  if (!message.empty()) ret.push_back(message);

#if DEBUG
  std::cout << "Parsed next message:";
  for (size_t i = 0; i < ret.size(); ++i) {
    std::cout << " " << ret[i];
  }
  std::cout << std::endl;
#endif

  return ret;
}  // namespace irc

void Chatbot::process_message_(const std::vector<std::string> &message) {
  if (!authenticated_) {
    if (message.size() && message[1] == "433")  // NICK already taken
    {
      ++instance_;
    } else if (irc_stringissame(message[0], "ping"))  // PING
    {
      std::stringstream answer;
      answer << "PONG";
      if (message.size() > 1) answer << " " << message[1];
      queue_.push(answer.str());
    } else if (message.size() && message[1] == "001")  // Welcome message
    {
      std::cout << "Authentication complete. Now online with username funbot"
                << instance_ << std::endl
                << "The bot now answers private messages to it which contain "
                   "the keywords 'JOKE' or 'joke' with a joke"
                << std::endl;
      authenticated_ = true;
    }
  } else {
    if (irc_stringissame(message[0], "ping"))  // PING
    {
      std::stringstream answer;
      answer << "PONG";
      if (message.size() > 1) answer << " " << message[1];
      queue_.push(answer.str());
    } else if (message.size() > 3 && irc_stringissame(message[1], "privmsg")) {
      std::string nick = extract_nick_(message[0]);
      if (message[3].find("joke") != std::string::npos ||
          message[3].find("JOKE") != std::string::npos) {
        // tell a joke
        std::stringstream answer;
        answer << "privmsg " << nick << " :Did I hear joke? Here's a good one: "
               << tell_random_joke_();
        queue_.push(answer.str());
      } else {
        // print instructions
        std::stringstream answer;
        answer << "privmsg " << nick
               << " :The funbot can only tell jokes. Ask for a joke!";
        queue_.push(answer.str());
      }
    }
  }
}

std::string Chatbot::tell_random_joke_() const {
  int random = rand() % 5;
  switch (random) {
    case 0:
      return "I've got a really funny UDP joke to tell you, but I'm not sure "
             "you'll get it.";
    case 1:
      return "How many programmers does it take to change a light bulb?  None. "
             "It's a hardware problem.";
    case 2:
      return "Why did the programmer die in the shower?  He read the shampoo "
             "bottle instructions: Lather. Rinse. Repeat.";
    case 3:
      return "Where is the best place to hide a body?  Page 2 of Google Search";
    default:
      return "Why do programmers prefer dark mode?  Because light attracts "
             "bugs.";
  }
}

std::string Chatbot::extract_nick_(std::string nickmask) {
  size_t end_of_nick = nickmask.find("!");

  if (end_of_nick == std::string::npos) return std::string();
  return nickmask.substr(1, end_of_nick - 1);
}

void Chatbot::send_authentication_request_(const std::string &password) {
  std::stringstream message;
  message << "PASS " << password << "\r\n"
          << "NICK funbot" << instance_ << "\r\n"
          << "USER comedybot 0 * :garfield\r\n";
  queue_.push(message.str());
  last_auth_try_ = time(NULL);
  std::cout << "Sent authentication request to server" << std::endl;
}

}  // namespace irc